#pragma once

#include <cstdint>

/// Runtime capture of QSystemLocale::QueryType traffic.
///
/// Qt installs a process-wide QSystemLocale (qlocale_p.h). Deriving from it is the
/// same mechanism platform plugins and Qt's own tests use, and lets a diagnostic
/// program record which QueryType values are actually queried by a given code path
/// without a debugger and without touching Qt6Core.dll.
namespace SpyLocale
{
void install();
std::uint64_t count(int queryType);
const char *name(int queryType);
void printHistogram();
}
