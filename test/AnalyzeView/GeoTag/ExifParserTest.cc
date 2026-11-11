#include "ExifParserTest.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <cstring>

#include "ExifParser.h"
#include "ExifUtility.h"
#include "GeoTagData.h"

void ExifParserTest::_readTimeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray imageBuffer = file.readAll();
    file.close();

    const qint64 imageTime = ExifParser::readTime(imageBuffer).toSecsSinceEpoch();

    const QDate date(2008, 10, 22);
    const QTime time(16, 28, 39);

    const QDateTime tagTime(date, time);
    const qint64 expectedTime = tagTime.toSecsSinceEpoch();

    QCOMPARE(imageTime, expectedTime);
}

void ExifParserTest::_readTimeEmptyBufferTest()
{
    const QByteArray emptyBuffer;
    expectLogMessage("AnalyzeView.ExifParser", QtWarningMsg, QRegularExpression("DateTimeDigitized tag not found"));
    const QDateTime result = ExifParser::readTime(emptyBuffer);
    verifyExpectedLogMessage();
    QVERIFY(!result.isValid());
}

void ExifParserTest::_readTimeInvalidJpegTest()
{
    const QByteArray invalidData("This is not a JPEG file");
    expectLogMessage("AnalyzeView.ExifParser", QtWarningMsg, QRegularExpression("DateTimeDigitized tag not found"));
    const QDateTime result = ExifParser::readTime(invalidData);
    verifyExpectedLogMessage();
    QVERIFY(!result.isValid());
}

void ExifParserTest::_writeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray imageBuffer = file.readAll();
    file.close();

    GeoTagData data;
    data.coordinate = QGeoCoordinate(37.225, -80.425, 618.4392);

    QVERIFY(ExifParser::write(imageBuffer, data));

    // Verify output is valid JPEG with EXIF
    QVERIFY(imageBuffer.size() > 0);
    QCOMPARE(static_cast<unsigned char>(imageBuffer[0]), static_cast<unsigned char>(0xFF));
    QCOMPARE(static_cast<unsigned char>(imageBuffer[1]), static_cast<unsigned char>(0xD8));
    QVERIFY(ExifUtility::hasExifData(imageBuffer));
}

void ExifParserTest::_writeEmptyBufferTest()
{
    QByteArray emptyBuffer;
    GeoTagData data;
    data.coordinate = QGeoCoordinate(37.225, -80.425, 618.4392);

    expectLogMessage("Utilities.ExifUtility", QtWarningMsg, QRegularExpression("Not a valid JPEG file"));
    // Writing to empty buffer should create new EXIF data
    const bool result = ExifParser::write(emptyBuffer, data);
    verifyExpectedLogMessage();
    // This may fail or succeed depending on implementation - just verify no crash
    Q_UNUSED(result);
}

void ExifParserTest::_writeAndReadBackTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Write GPS data using high-level API
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(-33.8688, 151.2093, 58.0);  // Sydney

    QVERIFY(ExifParser::write(buffer, geotag));

    // Read back using low-level API to verify
    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);
    ExifContent* gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    // Read latitude
    ExifEntry* latEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE));
    ExifEntry* latRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF));
    QVERIFY(latEntry != nullptr);
    QVERIFY(latRefEntry != nullptr);

    double latitude = ExifUtility::gpsRationalToDecimal(latEntry, order);
    if (latRefEntry->data[0] == 'S') {
        latitude = -latitude;
    }

    // Read longitude
    ExifEntry* lonEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE));
    ExifEntry* lonRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF));
    QVERIFY(lonEntry != nullptr);
    QVERIFY(lonRefEntry != nullptr);

    double longitude = ExifUtility::gpsRationalToDecimal(lonEntry, order);
    if (lonRefEntry->data[0] == 'W') {
        longitude = -longitude;
    }

    // Read altitude
    ExifEntry* altEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE));
    ExifEntry* altRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF));
    QVERIFY(altEntry != nullptr);
    QVERIFY(altRefEntry != nullptr);

    ExifRational altRational = exif_get_rational(altEntry->data, order);
    double altitude = static_cast<double>(altRational.numerator) / static_cast<double>(altRational.denominator);
    if (altRefEntry->data[0] == 1) {
        altitude = -altitude;
    }

    exif_data_unref(data);

    // Verify values within tolerance
    constexpr double coordTolerance = 0.001;
    constexpr double altTolerance = 0.1;

    QVERIFY2(qAbs(latitude - geotag.coordinate.latitude()) < coordTolerance,
             qPrintable(QString("Latitude: expected %1, got %2").arg(geotag.coordinate.latitude()).arg(latitude)));
    QVERIFY2(qAbs(longitude - geotag.coordinate.longitude()) < coordTolerance,
             qPrintable(QString("Longitude: expected %1, got %2").arg(geotag.coordinate.longitude()).arg(longitude)));
    QVERIFY2(qAbs(altitude - geotag.coordinate.altitude()) < altTolerance,
             qPrintable(QString("Altitude: expected %1, got %2").arg(geotag.coordinate.altitude()).arg(altitude)));
}

void ExifParserTest::_writeNegativeCoordinatesTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Test negative latitude (Southern hemisphere) and longitude (Western hemisphere)
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(-45.0312, -123.4567, 100.0);  // New Zealand / Pacific

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify using low-level API
    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifContent* gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    // Check reference values
    ExifEntry* latRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF));
    ExifEntry* lonRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF));

    QVERIFY(latRefEntry != nullptr);
    QVERIFY(lonRefEntry != nullptr);
    QCOMPARE(latRefEntry->data[0], static_cast<unsigned char>('S'));
    QCOMPARE(lonRefEntry->data[0], static_cast<unsigned char>('W'));

    exif_data_unref(data);
}

void ExifParserTest::_writeNegativeAltitudeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Test negative altitude (below sea level)
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(31.5, 35.5, -400.0);  // Dead Sea area

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify using low-level API
    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifContent* gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    ExifEntry* altRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF));
    QVERIFY(altRefEntry != nullptr);
    QCOMPARE(altRefEntry->data[0], static_cast<unsigned char>(1));  // 1 = below sea level

    exif_data_unref(data);
}

void ExifParserTest::_writePreservesExistingDataTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Read original timestamp before modifying
    const QDateTime originalTime = ExifParser::readTime(buffer);
    QVERIFY(originalTime.isValid());

    // Write GPS data
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(40.7128, -74.0060, 10.0);  // New York

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify original timestamp is preserved
    const QDateTime timeAfterWrite = ExifParser::readTime(buffer);
    QCOMPARE(timeAfterWrite, originalTime);
}

namespace {

/// Replaces the raw DateTimeDigitized tag value of a JPEG with \a value so that
/// readTime() sees an arbitrary (including malformed) EXIF timestamp string.
bool setDateTimeDigitizedValue(QByteArray& buffer, const QByteArray& value)
{
    if (value.size() > 19) {
        return false;
    }

    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        return false;
    }

    bool ok = false;
    ExifEntry* entry = ExifUtility::initTag(data, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED);
    if (entry && entry->data && (entry->size >= 20)) {
        memset(entry->data, 0, entry->size);
        memcpy(entry->data, value.constData(), static_cast<size_t>(value.size()));
        ok = ExifUtility::saveToBuffer(data, buffer);
    }

    exif_data_unref(data);
    return ok;
}

QByteArray loadTemplateJpeg()
{
    QFile file(":/unittest/DSCN0010.jpg");
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray buffer = file.readAll();
    file.close();
    return buffer;
}

}  // namespace

void ExifParserTest::_readTimeFormatTest()
{
    const QByteArray templateJpeg = loadTemplateJpeg();
    QVERIFY(!templateJpeg.isEmpty());

    // The empty tag value exercises the "no timestamp" warning path.
    ignoreLogMessage("AnalyzeView.ExifParser", QtWarningMsg, QRegularExpression("DateTimeDigitized tag not found"));

    const auto readInjected = [&templateJpeg](const QByteArray& tagValue) {
        QByteArray buffer = templateJpeg;
        if (!setDateTimeDigitizedValue(buffer, tagValue)) {
            return QDateTime();
        }
        return ExifParser::readTime(buffer);
    };

    // Specification-conforming value.
    {
        const QDateTime parsed = readInjected("2024:05:06 07:08:09");
        QVERIFY(parsed.isValid());
        QCOMPARE(parsed, QDateTime(QDate(2024, 5, 6), QTime(7, 8, 9)));
    }

    // Cameras that omit leading zeroes must still be understood.
    {
        const QDateTime parsed = readInjected("2024:5:6 7:8:9");
        QVERIFY(parsed.isValid());
        QCOMPARE(parsed, QDateTime(QDate(2024, 5, 6), QTime(7, 8, 9)));
    }

    // Leap day is only valid in a leap year.
    {
        const QDateTime leapDay = readInjected("2024:02:29 12:00:00");
        QVERIFY(leapDay.isValid());
        QCOMPARE(leapDay, QDateTime(QDate(2024, 2, 29), QTime(12, 0, 0)));

        QVERIFY(!readInjected("2023:02:29 12:00:00").isValid());
    }

    // Out-of-range and malformed values must be rejected instead of guessing.
    const QByteArray invalidValues[] = {
        "2024:13:06 07:08:09",   // month out of range
        "2024:00:06 07:08:09",   // month zero
        "2024:05:32 07:08:09",   // day out of range
        "2024:05:06 24:08:09",   // hour out of range
        "2024:05:06 07:60:09",   // minute out of range
        "2024:05:06 07:08:60",   // second out of range
        "2024:05:06 07:08",      // truncated
        "2024:05:06T07:08:09",   // wrong date/time separator
        "2024-05-06 07:08:09",   // wrong date separator
        "2024:05:06 07:08:09x",  // trailing garbage
        "24:05:06 07:08:09",     // two digit year
        ":2024:05:06 07:08:09",  // leading separator
        "",                      // empty value
    };
    for (const QByteArray& value : invalidValues) {
        QVERIFY2(!readInjected(value).isValid(), value.constData());
    }
}

void ExifParserTest::_readTimeConcurrentTest()
{
    // FGC-0029 regression: readTime() runs on every QtConcurrent EXIF worker thread.
    // It used to build a QDateTimeParser (via QDateTime::fromString), whose constructor
    // calls the process-global, unsynchronised QLocale::system(); concurrent workers then
    // freed the same locale string twice and died with STATUS_HEAP_CORRUPTION.
    const QByteArray imageBuffer = loadTemplateJpeg();
    QVERIFY(!imageBuffer.isEmpty());

    const QDateTime expected = ExifParser::readTime(imageBuffer);
    QVERIFY(expected.isValid());

    constexpr int kThreads = 12;
    constexpr int kIterations = 200;

    QList<int> work;
    work.reserve(kThreads * kIterations);
    for (int i = 0; i < (kThreads * kIterations); ++i) {
        work.append(i);
    }

    const QList<qint64> results = QtConcurrent::blockingMapped(
        work, [&imageBuffer](int) { return ExifParser::readTime(imageBuffer).toSecsSinceEpoch(); });

    QCOMPARE(results.count(), work.count());
    const qint64 expectedSeconds = expected.toSecsSinceEpoch();
    for (qsizetype i = 0; i < results.count(); ++i) {
        QCOMPARE(results.at(i), expectedSeconds);
    }
}

UT_REGISTER_TEST(ExifParserTest, TestLabel::Unit, TestLabel::AnalyzeView)