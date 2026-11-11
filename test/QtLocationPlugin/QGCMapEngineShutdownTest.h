#pragma once

#include "UnitTest.h"

/// FGC-0035: QGCMapEngine::shutdown() must never block forever.
///
/// The cache worker can be inside a SQLite call when shutdown is requested; a thread that
/// cannot be interrupted safely must not be joined without a bound, and must not be deleted
/// while it may still be using the database.
class QGCMapEngineShutdownTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRepeatedInitShutdown();
    void _testShutdownWithTaskInFlight();
    void _testShutdownUnderDatabaseLockContention();
};
