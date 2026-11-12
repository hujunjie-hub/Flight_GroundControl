#include "ExifParser.h"

#include <QtCore/QByteArray>
#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QTime>

#include "ExifUtility.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ExifParserLog, "AnalyzeView.ExifParser")

namespace {

/// Reads between \a minDigits and \a maxDigits ASCII digits starting at \a pos.
bool readDecimal(QStringView text, qsizetype& pos, qsizetype minDigits, qsizetype maxDigits, int& value)
{
    const qsizetype start = pos;
    int result = 0;

    while ((pos < text.size()) && ((pos - start) < maxDigits)) {
        const char16_t digit = text.at(pos).unicode();
        if ((digit < u'0') || (digit > u'9')) {
            break;
        }
        result = (result * 10) + static_cast<int>(digit - u'0');
        ++pos;
    }

    if ((pos - start) < minDigits) {
        return false;
    }

    value = result;
    return true;
}

/// Parses the fixed EXIF timestamp format `yyyy:MM:dd HH:mm:ss` (CIPA DC-008).
///
/// Deliberately does not use QDateTime::fromString().
///
/// FGC-0029: concurrent GeoTagController runs previously ended in STATUS_HEAP_CORRUPTION.
/// The captured exception stack terminated in QDateTime::fromString/QDateTimeParser::locale,
/// but that was the detection/free site and did not establish the first invalid access or free.
///
/// EXIF timestamps are locale-independent and have a fixed format. Parse that format directly
/// rather than invoking locale-dependent QDateTime::fromString on QtConcurrent worker threads.
/// The concurrent controller regression test exercises this path under parallel EXIF workloads.
/// Leading zeroes are mandated by the specification but not emitted by every camera, so one or
/// two digits are accepted for all fields but the year.
QDateTime parseExifTimestamp(QStringView text)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    qsizetype pos = 0;
    if (!readDecimal(text, pos, 4, 4, year) || (pos >= text.size()) || (text.at(pos).unicode() != u':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, month) || (pos >= text.size()) || (text.at(pos).unicode() != u':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, day) || (pos >= text.size()) || (text.at(pos).unicode() != u' ')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, hour) || (pos >= text.size()) || (text.at(pos).unicode() != u':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, minute) || (pos >= text.size()) || (text.at(pos).unicode() != u':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, second) || (pos != text.size())) {
        return QDateTime();
    }

    const QDate date(year, month, day);
    const QTime time(hour, minute, second);
    if (!date.isValid() || !time.isValid()) {
        return QDateTime();
    }

    // EXIF carries no time zone; keep the historical local-time interpretation.
    return QDateTime(date, time);
}

}  // namespace

namespace ExifParser {

QDateTime readTime(const QByteArray& buffer)
{
    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        qCWarning(ExifParserLog) << "Failed to parse EXIF data";
        return QDateTime();
    }

    const QString dateTimeDigitized = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
    exif_data_unref(data);

    if (dateTimeDigitized.isEmpty()) {
        qCWarning(ExifParserLog) << "DateTimeDigitized tag not found";
        return QDateTime();
    }

    return parseExifTimestamp(dateTimeDigitized);
}

bool write(QByteArray& buffer, const GeoTagData& geotag)
{
    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        data = ExifUtility::createNew();
        if (!data) {
            qCWarning(ExifParserLog) << "Failed to create EXIF data";
            return false;
        }
    }

    const ExifByteOrder order = exif_data_get_byte_order(data);

    ExifContent* gpsIfd = data->ifd[EXIF_IFD_GPS];
    if (!gpsIfd) {
        qCWarning(ExifParserLog) << "GPS IFD is unavailable";
        exif_data_unref(data);
        return false;
    }

    // GPS Version ID is exactly four BYTE components. Existing EXIF entries are
    // untrusted input: libexif preserves their declared format/count, so checking
    // only entry->data is not sufficient before writing four bytes.
    constexpr unsigned long kGpsVersionIdComponents = 4;
    constexpr size_t kGpsVersionIdSize = 4;
    ExifEntry* versionEntry = exif_content_get_entry(
        gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_VERSION_ID));
    if (!versionEntry) {
        versionEntry = ExifUtility::createTag(
            data, EXIF_IFD_GPS, static_cast<ExifTag>(EXIF_TAG_GPS_VERSION_ID),
            EXIF_FORMAT_BYTE, kGpsVersionIdComponents);
    }

    if (!versionEntry ||
        versionEntry->format != EXIF_FORMAT_BYTE ||
        versionEntry->components != kGpsVersionIdComponents ||
        !versionEntry->data ||
        versionEntry->size < kGpsVersionIdSize) {
        qCWarning(ExifParserLog) << "Malformed GPS Version ID EXIF entry";
        exif_data_unref(data);
        return false;
    }

    // The entry has been validated and is safe to update.
    versionEntry->data[0] = 2;
    versionEntry->data[1] = 3;
    versionEntry->data[2] = 0;
    versionEntry->data[3] = 0;

    // Helper to get existing or create new GPS entries.
    auto getOrCreateEntry = [&](ExifTag tag, ExifFormat format, unsigned long components) -> ExifEntry* {
        ExifEntry* entry = exif_content_get_entry(gpsIfd, tag);
        if (!entry) {
            entry = ExifUtility::createTag(data, EXIF_IFD_GPS, tag, format, components);
        }
        return entry;
    };

    // Latitude
    ExifEntry* latRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF), EXIF_FORMAT_ASCII, 2);
    ExifUtility::writeGpsRef(latRefEntry, geotag.coordinate.latitude() >= 0 ? 'N' : 'S');

    ExifEntry* latEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE), EXIF_FORMAT_RATIONAL, 3);
    ExifUtility::writeGpsCoordinate(latEntry, order, geotag.coordinate.latitude());

    // Longitude
    ExifEntry* lonRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF), EXIF_FORMAT_ASCII, 2);
    ExifUtility::writeGpsRef(lonRefEntry, geotag.coordinate.longitude() >= 0 ? 'E' : 'W');

    ExifEntry* lonEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE), EXIF_FORMAT_RATIONAL, 3);
    ExifUtility::writeGpsCoordinate(lonEntry, order, geotag.coordinate.longitude());

    // Altitude
    ExifEntry* altRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF), EXIF_FORMAT_BYTE, 1);
    ExifUtility::writeGpsAltRef(altRefEntry, geotag.coordinate.altitude() < 0 ? 1 : 0);

    ExifEntry* altEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE), EXIF_FORMAT_RATIONAL, 1);
    ExifUtility::writeRational(altEntry, order, geotag.coordinate.altitude(), 100);

    // Save back to buffer
    const bool success = ExifUtility::saveToBuffer(data, buffer);
    exif_data_unref(data);

    return success;
}

}  // namespace ExifParser
