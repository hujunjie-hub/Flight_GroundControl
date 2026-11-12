#include "ExifParser.h"

#include <QtCore/QByteArray>
#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QTime>

#include <cstring>
#include <string_view>

#include "ExifUtility.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ExifParserLog, "AnalyzeView.ExifParser")

namespace {

/// Reads between \a minDigits and \a maxDigits ASCII digits starting at \a pos.
bool readDecimal(std::string_view text, size_t& pos, size_t minDigits, size_t maxDigits, int& value)
{
    const size_t start = pos;
    int result = 0;

    while ((pos < text.size()) && ((pos - start) < maxDigits)) {
        const char digit = text[pos];
        if ((digit < '0') || (digit > '9')) {
            break;
        }
        result = (result * 10) + static_cast<int>(digit - '0');
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
QDateTime parseExifTimestamp(std::string_view text)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    size_t pos = 0;
    if (!readDecimal(text, pos, 4, 4, year) || (pos >= text.size()) || (text[pos] != ':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, month) || (pos >= text.size()) || (text[pos] != ':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, day) || (pos >= text.size()) || (text[pos] != ' ')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, hour) || (pos >= text.size()) || (text[pos] != ':')) {
        return QDateTime();
    }
    ++pos;
    if (!readDecimal(text, pos, 1, 2, minute) || (pos >= text.size()) || (text[pos] != ':')) {
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

    ExifEntry* dateTimeEntry = nullptr;
    if (data->ifd[EXIF_IFD_EXIF]) {
        dateTimeEntry = exif_content_get_entry(
            data->ifd[EXIF_IFD_EXIF], static_cast<ExifTag>(EXIF_TAG_DATE_TIME_DIGITIZED));
    }
    if (!dateTimeEntry) {
        dateTimeEntry = exif_data_get_entry(data, static_cast<ExifTag>(EXIF_TAG_DATE_TIME_DIGITIZED));
    }

    if (!dateTimeEntry) {
        qCWarning(ExifParserLog) << "DateTimeDigitized tag not found";
        exif_data_unref(data);
        return QDateTime();
    }

    // The EXIF DateTimeDigitized value is an ASCII byte sequence. Validate its
    // declared representation and bounds before inspecting the payload. Parsing
    // the bounded bytes directly avoids a temporary QString conversion on each
    // QtConcurrent worker and never scans beyond the libexif-owned entry buffer.
    if (dateTimeEntry->format != EXIF_FORMAT_ASCII ||
        !dateTimeEntry->data ||
        dateTimeEntry->components == 0 ||
        dateTimeEntry->components > 20 ||
        dateTimeEntry->size < dateTimeEntry->components) {
        qCWarning(ExifParserLog) << "Malformed DateTimeDigitized EXIF entry";
        exif_data_unref(data);
        return QDateTime();
    }

    const size_t valueSize = static_cast<size_t>(dateTimeEntry->components);
    const auto* terminator = static_cast<const unsigned char*>(
        std::memchr(dateTimeEntry->data, '\0', valueSize));
    if (!terminator) {
        qCWarning(ExifParserLog) << "Unterminated DateTimeDigitized EXIF entry";
        exif_data_unref(data);
        return QDateTime();
    }

    const std::string_view timestamp(
        reinterpret_cast<const char*>(dateTimeEntry->data),
        static_cast<size_t>(terminator - dateTimeEntry->data));
    const QDateTime result = parseExifTimestamp(timestamp);
    exif_data_unref(data);
    return result;
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

    // Helper to get existing or create new GPS entry
    auto getOrCreateEntry = [&](ExifTag tag, ExifFormat format, unsigned long components) -> ExifEntry* {
        ExifEntry* entry = exif_content_get_entry(data->ifd[EXIF_IFD_GPS], tag);
        if (!entry) {
            entry = ExifUtility::createTag(data, EXIF_IFD_GPS, tag, format, components);
        }
        return entry;
    };

    // GPS Version ID (2.3.0.0)
    ExifEntry* versionEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_VERSION_ID), EXIF_FORMAT_BYTE, 4);
    if (versionEntry && versionEntry->data) {
        versionEntry->data[0] = 2;
        versionEntry->data[1] = 3;
        versionEntry->data[2] = 0;
        versionEntry->data[3] = 0;
    }

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
