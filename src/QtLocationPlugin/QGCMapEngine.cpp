#include "QGCMapEngine.h"

#include <QtCore/QApplicationStatic>

#include "QGCCachedTileSet.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTile.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"
#include "QGeoFileTileCacheQGC.h"

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "QtLocationPlugin.QGCMapEngine")

namespace {
/// Upper bound for stopping the cache worker during shutdown (FGC-0035). Long enough for a
/// database operation to finish, short enough that a stuck worker cannot hang process exit.
constexpr int kWorkerShutdownTimeoutMs = 5000;
} // namespace

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

QGCMapEngine *getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine::QGCMapEngine(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGCMapEngineLog) << this;

    (void) qRegisterMetaType<QGCMapTask::TaskType>("QGCMapTask::TaskType");
    (void) qRegisterMetaType<QGCTile>("QGCTile");
    (void) qRegisterMetaType<QGCTile*>("QGCTile*");
    (void) qRegisterMetaType<QList<QGCTile*>>("QList<QGCTile*>");
    (void) qRegisterMetaType<QGCTileSet>("QGCTileSet");
    (void) qRegisterMetaType<QGCTileSet*>("QGCTileSet*");
    (void) qRegisterMetaType<QGCCacheTile>("QGCCacheTile");
    (void) qRegisterMetaType<QGCCacheTile*>("QGCCacheTile*");
}

QGCMapEngine::~QGCMapEngine()
{
    shutdown();

    qCDebug(QGCMapEngineLog) << this;
}

void QGCMapEngine::shutdown()
{
    // Tear down whenever a worker exists, even if init() failed partway and
    // cleared m_initialized - otherwise the worker thread would leak.
    if (m_worker) {
        qCInfo(QGCMapEngineLog) << "shutdown: disconnecting and stopping cache worker" << m_worker
                                << "running:" << m_worker->isRunning();
        (void) disconnect(m_worker);
        m_worker->stop();
        qCInfo(QGCMapEngineLog) << "shutdown: stop() returned; waiting up to" << kWorkerShutdownTimeoutMs << "ms";

        // Bounded wait (FGC-0035). The worker may be inside a SQLite call it cannot abandon, so
        // an unbounded wait() can hang the process forever - including in a static destructor,
        // where nothing can recover. The contract is: shutdown gives the worker
        // kWorkerShutdownTimeoutMs to finish, then reports it and stops owning the thread.
        const bool workerStopped = m_worker->wait(kWorkerShutdownTimeoutMs);
        qCInfo(QGCMapEngineLog) << "shutdown: worker wait completed; stopped:" << workerStopped;
        if (workerStopped) {
            delete m_worker;
        } else {
            // Deliberately do NOT delete the worker: it is still running and may still be using
            // the database. Detach it from this engine as well - QObject::~QObject would
            // otherwise delete the child and ~QGCCacheWorker's own wait() would block process
            // exit forever (that is exactly the FGC-0035 hang). The thread deletes itself when
            // it finally finishes; at process exit the OS reclaims it.
            qCWarning(QGCMapEngineLog)
                << "cache worker did not stop within" << kWorkerShutdownTimeoutMs
                << "ms; detaching it and deleting it when the thread finishes";
            QGCCacheWorker *const orphan = m_worker;
            orphan->setParent(nullptr);
            (void) connect(orphan, &QThread::finished, orphan, &QObject::deleteLater);
        }
        m_worker = nullptr;
    }
    m_initialized = false;
}

QGCMapEngine *QGCMapEngine::instance()
{
    return _mapEngine();
}

void QGCMapEngine::init(const QString &databasePath)
{
    if (m_initialized) {
        return;
    }

    m_initialized = true;

    m_worker = new QGCCacheWorker(this);
    m_worker->setDatabaseFile(databasePath);
    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);

    QGCMapTask *task = new QGCMapTask(QGCMapTask::TaskType::taskInit);
    if (!addTask(task)) {
        task->deleteLater();
        m_initialized = false;
    }
}

bool QGCMapEngine::addTask(QGCMapTask *task)
{
    if (!m_worker) {
        return false;
    }

    // DirectConnection is intentional: the worker thread uses a custom loop (not
    // an event loop), so queued connections would never be delivered. The queue
    // is mutex-protected in enqueueTask, so calling from the main thread is safe.
    bool result = false;
    (void) QMetaObject::invokeMethod(m_worker, &QGCCacheWorker::enqueueTask, Qt::DirectConnection, qReturnArg(result), task);
    return result;
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint64 maxSize = static_cast<quint64>(QGeoFileTileCacheQGC::getMaxDiskCacheSetting()) * qPow(1024, 2);
    if (!m_pruning && (defaultsize > maxSize)) {
        m_pruning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCPruneCacheTask *task = new QGCPruneCacheTask(amountToPrune);
        (void) connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        if (!addTask(task)) {
            task->deleteLater();
            m_pruning = false;
        }
    }
}
