#include "QGCMapEngineShutdownTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtTest/QTest>

#include "QGCCacheTile.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"

namespace {

/// QGCMapEngine::shutdown() must return within its documented bound. Keep a little slack for
/// a loaded machine, but this is far below the 60 s CTest timeout on purpose: the regression
/// this guards against was an unbounded QThread::wait().
constexpr qint64 kShutdownBudgetMs = 15000;

QString _providerType()
{
    return QStringLiteral("Bing Road");
}

} // namespace

void QGCMapEngineShutdownTest::_testRepeatedInitShutdown()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    for (int cycle = 0; cycle < 20; ++cycle) {
        const QString dbPath = tempDir.filePath(QStringLiteral("cycle_%1.db").arg(cycle));
        QElapsedTimer timer;
        timer.start();

        getQGCMapEngine()->init(dbPath);
        getQGCMapEngine()->shutdown();

        const qint64 elapsed = timer.elapsed();
        QVERIFY2(elapsed < kShutdownBudgetMs,
                 qPrintable(QStringLiteral("cycle %1: init/shutdown took %2 ms (budget %3 ms)")
                                .arg(cycle).arg(elapsed).arg(kShutdownBudgetMs)));
    }
}

void QGCMapEngineShutdownTest::_testShutdownWithTaskInFlight()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath(QStringLiteral("inflight.db"));

    getQGCMapEngine()->init(dbPath);

    // Queue enough database work that shutdown is very likely to race with a task.
    for (int i = 0; i < 200; ++i) {
        auto *tile = new QGCCacheTile(QStringLiteral("inflight_%1").arg(i), QByteArray(64, 'I'),
                                      QStringLiteral("png"), _providerType());
        getQGCMapEngine()->addTask(new QGCSaveTileTask(tile));
    }

    QElapsedTimer timer;
    timer.start();
    getQGCMapEngine()->shutdown();
    const qint64 elapsed = timer.elapsed();

    QVERIFY2(elapsed < kShutdownBudgetMs,
             qPrintable(QStringLiteral("shutdown with tasks in flight took %1 ms (budget %2 ms)")
                            .arg(elapsed).arg(kShutdownBudgetMs)));
}

void QGCMapEngineShutdownTest::_testShutdownUnderDatabaseLockContention()
{
    // The blocked connection makes the worker's schema write fail; that warning is the
    // expected outcome of this scenario, not a defect.
    ignoreLogMessage("QtLocationPlugin.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to set schema version")));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath(QStringLiteral("locked.db"));

    // Create the database first so the blocking connection can lock the same file.
    getQGCMapEngine()->init(dbPath);
    getQGCMapEngine()->shutdown();

    // Hold an exclusive write transaction from this thread: the worker's next write has to
    // wait for the SQLite lock (bounded by the busy timeout) instead of returning instantly.
    const QString blockerConnection = QStringLiteral("qgc_shutdown_blocker");
    {
        QSqlDatabase blocker = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), blockerConnection);
        blocker.setDatabaseName(dbPath);
        QVERIFY(blocker.open());
        QSqlQuery q(blocker);
        QVERIFY(q.exec(QStringLiteral("PRAGMA journal_mode=WAL")));
        QVERIFY(q.exec(QStringLiteral("BEGIN IMMEDIATE")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS blocker (id INTEGER)")));

        getQGCMapEngine()->init(dbPath);
        for (int i = 0; i < 50; ++i) {
            auto *tile = new QGCCacheTile(QStringLiteral("locked_%1").arg(i), QByteArray(64, 'L'),
                                          QStringLiteral("png"), _providerType());
            getQGCMapEngine()->addTask(new QGCSaveTileTask(tile));
        }

        QElapsedTimer timer;
        timer.start();
        getQGCMapEngine()->shutdown();
        const qint64 elapsed = timer.elapsed();

        QVERIFY2(elapsed < kShutdownBudgetMs,
                 qPrintable(QStringLiteral("shutdown under lock contention took %1 ms (budget %2 ms)")
                                .arg(elapsed).arg(kShutdownBudgetMs)));

        // Release the lock only after shutdown returned; the worker must not have been
        // deleted while it could still be using the database.
        QVERIFY(q.exec(QStringLiteral("ROLLBACK")));
        blocker.close();
    }
    QSqlDatabase::removeDatabase(blockerConnection);
}

UT_REGISTER_TEST(QGCMapEngineShutdownTest, TestLabel::Unit)
