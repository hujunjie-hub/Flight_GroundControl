// Runtime QueryType capture for the FGC-0029 differential experiment.
//
// Qt 6.11.1 qlocale_p.h declares
//     virtual QVariant query(QueryType type, QVariant &&in = QVariant()) const;
// as a public virtual on the exported QSystemLocale class, and QSystemLocale's
// constructor installs itself as the process-wide system locale. Deriving from it
// therefore redirects every system-locale query through this counter, while still
// delegating to the real Windows backend.

#include "spy_systemlocale.h"

#include <QtCore/QVariant>

#include <private/qlocale_p.h>

#include <atomic>
#include <cstdio>

namespace
{
std::atomic<std::uint64_t> g_counts[64];

class SpySystemLocale final : public QSystemLocale
{
public:
    QVariant query(QueryType type, QVariant &&in = QVariant()) const override
    {
        const int index = static_cast<int>(type);
        if (index >= 0 && index < 64) {
            g_counts[index].fetch_add(1, std::memory_order_relaxed);
        }
        return QSystemLocale::query(type, std::move(in));
    }
};

const char *const kNames[] = {
    "LanguageId", "TerritoryId", "DecimalPoint", "Grouping", "GroupSeparator", "ZeroDigit",
    "NegativeSign", "DateFormatLong", "DateFormatShort", "TimeFormatLong", "TimeFormatShort",
    "DayNameLong", "DayNameShort", "DayNameNarrow", "MonthNameLong", "MonthNameShort",
    "MonthNameNarrow", "DateToStringLong", "DateToStringShort", "TimeToStringLong",
    "TimeToStringShort", "DateTimeFormatLong", "DateTimeFormatShort", "DateTimeToStringLong",
    "DateTimeToStringShort", "MeasurementSystem", "PositiveSign", "AMText", "PMText",
    "FirstDayOfWeek", "Weekdays", "CurrencySymbol", "CurrencyToString", "Collation",
    "UILanguages", "StringToStandardQuotation", "StringToAlternateQuotation", "ScriptId",
    "ListToSeparatedString", "LocaleChanged", "NativeLanguageName", "NativeTerritoryName",
    "StandaloneMonthNameLong", "StandaloneMonthNameShort", "StandaloneMonthNameNarrow",
    "StandaloneDayNameLong", "StandaloneDayNameShort", "StandaloneDayNameNarrow",
};
constexpr int kNameCount = static_cast<int>(sizeof(kNames) / sizeof(kNames[0]));
} // namespace

namespace SpyLocale
{

void install()
{
    // Intentionally leaked: QSystemLocale's destructor unlinks itself, and this object
    // must stay installed until process exit.
    new SpySystemLocale;
}

std::uint64_t count(int queryType)
{
    if (queryType < 0 || queryType >= 64) {
        return 0;
    }
    return g_counts[queryType].load(std::memory_order_relaxed);
}

const char *name(int queryType)
{
    if (queryType >= 0 && queryType < kNameCount) {
        return kNames[queryType];
    }
    return "Unknown";
}

void printHistogram()
{
    std::printf("[spy] QSystemLocale::query histogram\n");
    for (int i = 0; i < 64; ++i) {
        const std::uint64_t n = count(i);
        if (n != 0) {
            std::printf("[spy]   %-2d %-28s %llu\n", i, name(i),
                        static_cast<unsigned long long>(n));
        }
    }
    std::fflush(stdout);
}

} // namespace SpyLocale
