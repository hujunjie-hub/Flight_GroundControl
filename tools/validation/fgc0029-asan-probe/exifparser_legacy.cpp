// Test-only copy of the pre-fix ExifParser::readTime() body.
//
// Source: src/AnalyzeView/GeoTag/ExifParser.cc at commit
//         8117cfed79a36ab3f5b3c5771096220bb05ef588 (R16), lines 13-30.
// The only edit is the namespace (ExifParserLegacy instead of ExifParser) and the
// logging category, so that this diagnostic control never links into production code.
//
// Keep this file byte-identical to the quoted commit apart from those two changes;
// the differential experiment depends on it calling QDateTime::fromString() exactly
// the way the product did before 8295dc0.

#include "exifparser_legacy.h"

#include "ExifUtility.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ExifParserLegacyLog, "AnalyzeView.ExifParser.Legacy")

namespace ExifParserLegacy
{

QDateTime readTime(const QByteArray &buffer)
{
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        qCWarning(ExifParserLegacyLog) << "Failed to parse EXIF data";
        return QDateTime();
    }

    const QString dateTimeDigitized = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
    exif_data_unref(data);

    if (dateTimeDigitized.isEmpty()) {
        qCWarning(ExifParserLegacyLog) << "DateTimeDigitized tag not found";
        return QDateTime();
    }

    return QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
}

} // namespace ExifParserLegacy
