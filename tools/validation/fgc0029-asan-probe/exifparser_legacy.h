#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>

/// Test-only copy of ExifParser::readTime() as it existed at commit 8117cfed
/// (the commit immediately before the FGC-0029 fix 8295dc0), used as the C arm of
/// the differential experiment. The production ExifParser is NOT modified.
namespace ExifParserLegacy
{
QDateTime readTime(const QByteArray &buffer);
}
