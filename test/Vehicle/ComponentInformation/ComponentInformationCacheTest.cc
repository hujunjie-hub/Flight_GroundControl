#include "ComponentInformationCacheTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

#include "ComponentInformationCache.h"
#include "UnitTest.h"

ComponentInformationCacheTest::ComponentInformationCacheTest()
{
    const QString uniqueSuffix = QStringLiteral("%1_%2")
                                     .arg(QCoreApplication::applicationPid())
                                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    _cacheDir = tempRoot + QLatin1String("/QGCCacheTest_") + uniqueSuffix;
    _tmpFilesDir = tempRoot + QLatin1String("/QGCTestFiles_") + uniqueSuffix;
    _cleanup();
}

void ComponentInformationCacheTest::_setup()
{
    QDir d;
    QVERIFY(d.mkpath(_tmpFilesDir));
    _tmpFiles.clear();
    for (int i = 0; i < 30; ++i) {
        TmpFile t;
        t.content = QString::asprintf("%i", i);
        t.path = _tmpFilesDir + "/" + t.content + ".txt";
        t.cacheTag = QString::asprintf("_tag_%08i_xy", i);
        QFile f(t.path);
        if (f.open(QIODevice::WriteOnly)) {
            (void)f.write(t.content.toUtf8().constData(), t.content.toUtf8().size());
            f.close();
        } else {
            qWarning() << "Error opening file" << f.fileName();
        }
        _tmpFiles.push_back(t);
    }
}

void ComponentInformationCacheTest::_cleanup()
{
    QDir t(_tmpFilesDir);
    t.removeRecursively();
    QDir d(_cacheDir);
    d.removeRecursively();
}

void ComponentInformationCacheTest::_basic_test()
{
    _setup();
    ComponentInformationCache cache(_cacheDir, 10);
    QDir cacheDir(_cacheDir);
    QVERIFY(cacheDir.exists());
    _tmpFiles[0].cachedPath = cache.insert(_tmpFiles[0].cacheTag, _tmpFiles[0].path);
    QVERIFY(!_tmpFiles[0].cachedPath.isEmpty());
    QVERIFY(QFile(_tmpFiles[0].cachedPath).exists());
    QVERIFY(!QFile(_tmpFiles[0].path).exists());
    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == _tmpFiles[0].cachedPath);
    QFile f(_tmpFiles[0].cachedPath);
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    QTextStream in(&f);
    QVERIFY(in.readAll() == _tmpFiles[0].content);
    // Close before cleanup: an open handle keeps the cache file alive on Windows and
    // leaks a stale data-without-meta entry into the next test function.
    f.close();
    _cleanup();
}

void ComponentInformationCacheTest::_lru_test()
{
    _setup();
    ComponentInformationCache cache(_cacheDir, 3);
    auto insert = [&](int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QVERIFY(!_tmpFiles[idx].cachedPath.isEmpty());
    };
    insert(1);
    insert(3);
    insert(0);
    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == _tmpFiles[0].cachedPath);
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    insert(4);
    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == _tmpFiles[4].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    insert(5);
    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[5].cacheTag) == _tmpFiles[5].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    insert(6);
    insert(7);
    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[5].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[6].cacheTag) == _tmpFiles[6].cachedPath);
    QVERIFY(cache.access(_tmpFiles[7].cacheTag) == _tmpFiles[7].cachedPath);
    _cleanup();
}

void ComponentInformationCacheTest::_orphanDataFileRemoved_test()
{
    // A data file without metadata can only be a leftover of an interrupted insert.
    // It is inaccessible and must not block a later insert() for the same tag.
    QVERIFY(QDir().mkpath(_cacheDir));
    const QString orphanPath = _cacheDir + "/" + QStringLiteral("_tag_orphan_xy") + QStringLiteral(".cache");
    QFile orphan(orphanPath);
    QVERIFY(orphan.open(QIODevice::WriteOnly));
    orphan.write("orphan");
    orphan.close();

    expectLogMessage("ComponentInformation.ComponentInformationCache", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Removing orphan cache file without metadata")));
    ComponentInformationCache cache(_cacheDir, 10);
    QVERIFY2(!QFile::exists(orphanPath), "Orphan cache data file must be removed on initialization");
    verifyExpectedLogMessage();

    _setup();
    const QString cachedPath = cache.insert(QStringLiteral("_tag_orphan_xy"), _tmpFiles[0].path);
    QVERIFY(!cachedPath.isEmpty());
    QCOMPARE(cache.access(QStringLiteral("_tag_orphan_xy")), cachedPath);
    QVERIFY(QFile::exists(cachedPath));
    _cleanup();
}

void ComponentInformationCacheTest::_incompleteEntrySelfHeals_test()
{
    _setup();
    ComponentInformationCache cache(_cacheDir, 10);
    const QString tag = _tmpFiles[0].cacheTag;
    const QString firstPath = cache.insert(tag, _tmpFiles[0].path);
    QVERIFY(!firstPath.isEmpty());
    QVERIFY(QFile::exists(firstPath));

    // Simulate a torn write: the metadata disappears while the data stays.
    QFile meta(_cacheDir + "/" + tag + QStringLiteral(".meta"));
    QVERIFY(meta.exists());
    QVERIFY(meta.remove());
    QCOMPARE(cache.access(tag), QString());

    // Re-inserting the same tag must repair the entry instead of reporting success
    // while leaving it inaccessible.
    expectLogMessage("ComponentInformation.ComponentInformationCache", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Removing incomplete cache entry")));
    const QString repairedPath = cache.insert(tag, _tmpFiles[1].path);
    verifyExpectedLogMessage();
    QVERIFY(!repairedPath.isEmpty());
    QCOMPARE(cache.access(tag), repairedPath);
    QFile data(repairedPath);
    QVERIFY(data.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(data.readAll()), _tmpFiles[1].content);
    data.close();
    _cleanup();
}
void ComponentInformationCacheTest::_multi_test()
{
    _setup();
    auto insert = [&](ComponentInformationCache& cache, int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QVERIFY(!_tmpFiles[idx].cachedPath.isEmpty());
    };
    {
        ComponentInformationCache cache(_cacheDir, 5);
        for (int i = 0; i < 5; ++i) {
            insert(cache, i);
            QVERIFY(cache.access(_tmpFiles[i].cacheTag) == _tmpFiles[i].cachedPath);
        }
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    }
    {
        // reduce cache size and ensure oldest entries are evicted
        ComponentInformationCache cache(_cacheDir, 3);
        QVERIFY(cache.access(_tmpFiles[0].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
        QVERIFY(cache.access(_tmpFiles[2].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
        QVERIFY(cache.access(_tmpFiles[4].cacheTag) == _tmpFiles[4].cachedPath);
        insert(cache, 10);
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[10].cacheTag) == _tmpFiles[10].cachedPath);
    }
    _cleanup();
}

UT_REGISTER_TEST(ComponentInformationCacheTest, TestLabel::Unit, TestLabel::Vehicle)
