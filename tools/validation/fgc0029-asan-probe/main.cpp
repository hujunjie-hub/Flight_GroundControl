// ============================================================================
// FGC-0029 ASan differential probe.
//
// Diagnostic-only harness. It compiles the *unmodified* production sources
// (src/AnalyzeView/GeoTag/ExifParser.cc, src/Utilities/Parsing/Exif/ExifUtility.cc)
// plus the pinned libexif sources, all with MSVC /fsanitize=address, and drives them
// through the same stages the GeoTag pipeline uses so a heap error can be attributed
// to one stage instead of "somewhere in EXIF parsing".
//
// Modes:
//   a  QDateTime::fromString only              (Qt only)
//   b  libexif loadFromBuffer/readString/unref (libexif only)
//   c  ExifParser::readTime                    (libexif + Qt)
//   d  QFile read + ExifParser::readTime       (controller worker shape)
//   e  QtConcurrent::mapped + QFutureWatcher + shared cache + ExifParser::write
//                                              (controller async shape, write path)
//   qlocale  QLocale::system() only            (R18 differential experiment A)
//   legacy   pre-fix readTime body             (R18 differential experiment C: the
//            QDateTime::fromString call that 8295dc0 removed; diagnostic control only)
//
// Usage: fgc0029_asan_probe --mode=c --threads=12 --iterations=200 --image=<file.jpg>
// ============================================================================

#include "ExifParser.h"
#include "ExifUtility.h"
#include "GeoTagData.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QLocale>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>

#include <atomic>
#include <cstdio>
#include <functional>
#include <thread>
#include <vector>

// The product build links QGCLoggingCategory.cc, which depends on the QGC logging
// manager. The probe only needs the category objects to exist; registration is
// irrelevant for memory diagnostics, so the constructor is satisfied locally.
QGCLoggingCategory::QGCLoggingCategory(const QString &category)
{
    Q_UNUSED(category);
}

namespace {

struct Options {
    QString mode;
    int threads = 1;
    int iterations = 100;
    QString image;
    bool verbose = false;
};

struct Result {
    int index = 0;
    qint64 timestamp = 0;
    bool success = false;
    QString error;
};

std::atomic<long long> g_ok{0};
std::atomic<long long> g_fail{0};

void reportFailure(const char *what)
{
    const long long n = g_fail.fetch_add(1) + 1;
    if (n <= 5) {
        std::printf("[probe] failure #%lld: %s\n", n, what);
        std::fflush(stdout);
    }
}

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

QByteArray readWholeFile(const QString &path, bool *ok)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *ok = false;
        return {};
    }
    const QByteArray data = file.readAll();
    *ok = !data.isEmpty();
    return data;
}

void runThreads(const Options &opt, const std::function<void(int)> &body)
{
    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(opt.threads));
    for (int t = 0; t < opt.threads; ++t) {
        workers.emplace_back([&body, t]() { body(t); });
    }
    for (std::thread &w : workers) {
        w.join();
    }
}

// ---------------------------------------------------------------------------
// a: QDateTime::fromString only
// ---------------------------------------------------------------------------

void runModeA(const Options &opt)
{
    static const char *const kSamples[] = {
        "2024:05:06 07:08:09",
        "2023:12:31 23:59:59",
        "1999:01:01 00:00:00",
        "2026:11:11 09:12:23",
    };

    runThreads(opt, [&opt](int tid) {
        for (int i = 0; i < opt.iterations; ++i) {
            const QString text = QString::fromLatin1(kSamples[(tid + i) % 4]);
            const QDateTime dt = QDateTime::fromString(text, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
            if (!dt.isValid()) {
                reportFailure("QDateTime::fromString returned an invalid datetime");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// b: libexif only
// ---------------------------------------------------------------------------

void runModeB(const Options &opt, const QByteArray &image)
{
    runThreads(opt, [&opt, &image](int) {
        for (int i = 0; i < opt.iterations; ++i) {
            ExifData *data = ExifUtility::loadFromBuffer(image);
            if (!data) {
                reportFailure("loadFromBuffer returned null");
                continue;
            }
            const QString value = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
            exif_data_unref(data);
            if (value.isEmpty()) {
                reportFailure("readString(DateTimeDigitized) empty");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// c: ExifParser::readTime
// ---------------------------------------------------------------------------

void runModeC(const Options &opt, const QByteArray &image)
{
    runThreads(opt, [&opt, &image](int) {
        for (int i = 0; i < opt.iterations; ++i) {
            const QDateTime dt = ExifParser::readTime(image);
            if (!dt.isValid()) {
                reportFailure("readTime returned an invalid datetime");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// qlocale: QLocale::system() only (FGC-0029 R18 differential experiment A)
// ---------------------------------------------------------------------------

void runModeQSystemLocale(const Options &opt)
{
    runThreads(opt, [&opt](int) {
        for (int i = 0; i < opt.iterations; ++i) {
            const QLocale locale = QLocale::system();
            // Force the returned QLocale to be materialised so the call is not elided.
            if (locale.language() == QLocale::AnyLanguage) {
                reportFailure("QLocale::system() returned AnyLanguage");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// legacy: byte-for-byte replication of the pre-fix readTime() body
//         (commit 8117cfe ExifParser::readTime, kept only as a diagnostic control)
// ---------------------------------------------------------------------------

void runModeLegacy(const Options &opt, const QByteArray &image)
{
    runThreads(opt, [&opt, &image](int) {
        for (int i = 0; i < opt.iterations; ++i) {
            ExifData *data = ExifUtility::loadFromBuffer(image);
            if (!data) {
                reportFailure("legacy: loadFromBuffer returned null");
                continue;
            }
            const QString text = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
            exif_data_unref(data);
            if (text.isEmpty()) {
                reportFailure("legacy: empty DateTimeDigitized");
                continue;
            }
            const QDateTime dt = QDateTime::fromString(text, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
            if (!dt.isValid()) {
                reportFailure("legacy: QDateTime::fromString returned an invalid datetime");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// d: file read + readTime (mirrors GeoTagController::_parseExifForImage)
// ---------------------------------------------------------------------------

void runModeD(const Options &opt)
{
    runThreads(opt, [&opt](int) {
        for (int i = 0; i < opt.iterations; ++i) {
            bool readOk = false;
            const QByteArray buffer = readWholeFile(opt.image, &readOk);
            if (!readOk) {
                reportFailure("could not read image file");
                continue;
            }
            const QDateTime dt = ExifParser::readTime(buffer);
            if (!dt.isValid()) {
                reportFailure("readTime returned an invalid datetime");
                continue;
            }
            g_ok.fetch_add(1, std::memory_order_relaxed);
        }
    });
}

// ---------------------------------------------------------------------------
// e: QtConcurrent + QFutureWatcher + shared buffer cache + write path
// ---------------------------------------------------------------------------

void runModeE(const Options &opt, const QByteArray &image)
{
    // One file per worker so every task does real disk I/O, like the real pipeline.
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        std::printf("[probe] could not create temp dir\n");
        std::exit(2);
    }

    QStringList paths;
    paths.reserve(opt.threads);
    for (int i = 0; i < opt.threads; ++i) {
        const QString path = tempDir.filePath(QStringLiteral("image_%1.jpg").arg(i, 3, 10, QChar('0')));
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly) || out.write(image) != image.size()) {
            std::printf("[probe] could not prepare %s\n", qPrintable(path));
            std::exit(2);
        }
        out.close();
        paths.append(path);
    }

    QHash<QString, QByteArray> bufferCache;
    QMutex bufferMutex;

    auto readCached = [&](const QString &path) -> QByteArray {
        {
            QMutexLocker locker(&bufferMutex);
            const auto it = bufferCache.constFind(path);
            if (it != bufferCache.constEnd()) {
                return it.value();
            }
        }
        bool ok = false;
        const QByteArray data = readWholeFile(path, &ok);
        if (!ok) {
            return {};
        }
        {
            QMutexLocker locker(&bufferMutex);
            bufferCache.insert(path, data);
        }
        return data;
    };

    GeoTagData geotag;
    geotag.timestamp = 1700000000;
    geotag.coordinate = QGeoCoordinate(37.123456, -122.654321, 123.5);
    geotag.captureResult = GeoTagData::CaptureResult::Success;

    for (int round = 0; round < opt.iterations; ++round) {
        // Stage 1: parallel EXIF timestamp parsing (GeoTagController::_startParseExif).
        {
            QFuture<Result> future = QtConcurrent::mapped(paths, [&](const QString &path) {
                Result r;
                const QByteArray buffer = readCached(path);
                if (buffer.isEmpty()) {
                    r.error = QStringLiteral("empty buffer");
                    return r;
                }
                const QDateTime dt = ExifParser::readTime(buffer);
                r.timestamp = dt.isValid() ? dt.toSecsSinceEpoch() : 0;
                r.success = dt.isValid();
                return r;
            });

            QFutureWatcher<Result> watcher;
            QEventLoop loop;
            QObject::connect(&watcher, &QFutureWatcher<Result>::finished, &loop, &QEventLoop::quit);
            watcher.setFuture(future);
            if (!future.isFinished()) {
                loop.exec();
            }
            for (const Result &r : future.results()) {
                if (r.success) {
                    g_ok.fetch_add(1, std::memory_order_relaxed);
                } else {
                    reportFailure(qPrintable(r.error));
                }
            }
        }

        // Stage 2: parallel tagging (GeoTagController::_startTagImages / _tagImage).
        {
            const QString outDir = tempDir.filePath(QStringLiteral("tagged"));
            QDir().mkpath(outDir);

            QFuture<Result> future = QtConcurrent::mapped(paths, [&](const QString &path) {
                Result r;
                QByteArray buffer = readCached(path);
                if (buffer.isEmpty()) {
                    r.error = QStringLiteral("empty buffer");
                    return r;
                }
                buffer.detach();
                if (!ExifParser::write(buffer, geotag)) {
                    r.error = QStringLiteral("ExifParser::write failed");
                    return r;
                }
                const QString outPath = QDir(outDir).filePath(QFileInfo(path).fileName());
                QFile out(outPath);
                if (!out.open(QIODevice::WriteOnly) || out.write(buffer) != buffer.size()) {
                    r.error = QStringLiteral("could not write tagged image");
                    return r;
                }
                r.success = true;
                return r;
            });

            QFutureWatcher<Result> watcher;
            QEventLoop loop;
            QObject::connect(&watcher, &QFutureWatcher<Result>::finished, &loop, &QEventLoop::quit);
            watcher.setFuture(future);
            if (!future.isFinished()) {
                loop.exec();
            }
            for (const Result &r : future.results()) {
                if (r.success) {
                    g_ok.fetch_add(1, std::memory_order_relaxed);
                } else {
                    reportFailure(qPrintable(r.error));
                }
            }
        }
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Options opt;
    const QStringList args = QCoreApplication::arguments();
    for (const QString &arg : args) {
        if (arg.startsWith(QStringLiteral("--mode="))) {
            opt.mode = arg.mid(7);
        } else if (arg.startsWith(QStringLiteral("--threads="))) {
            opt.threads = arg.mid(10).toInt();
        } else if (arg.startsWith(QStringLiteral("--iterations="))) {
            opt.iterations = arg.mid(13).toInt();
        } else if (arg.startsWith(QStringLiteral("--image="))) {
            opt.image = arg.mid(8);
        } else if (arg == QStringLiteral("--verbose")) {
            opt.verbose = true;
        }
    }

    if (opt.mode.isEmpty() || opt.threads < 1 || opt.iterations < 1) {
        std::printf("usage: %s --mode=a|b|c|d|e --threads=N --iterations=N --image=file.jpg\n",
                    argv[0]);
        return 2;
    }

    if (opt.image.isEmpty()) {
        std::printf("[probe] --image is required\n");
        return 2;
    }

    QFileInfo imageInfo(opt.image);
    if (!imageInfo.exists()) {
        std::printf("[probe] image not found: %s\n", qPrintable(opt.image));
        return 2;
    }

    bool readOk = false;
    const QByteArray image = readWholeFile(opt.image, &readOk);
    if (!readOk) {
        std::printf("[probe] could not read image: %s\n", qPrintable(opt.image));
        return 2;
    }

    std::printf("[probe] mode=%s threads=%d iterations=%d image=%s bytes=%lld\n",
                qPrintable(opt.mode), opt.threads, opt.iterations,
                qPrintable(imageInfo.fileName()), static_cast<long long>(image.size()));
    std::fflush(stdout);

    QElapsedTimer timer;
    timer.start();

    if (opt.mode == QStringLiteral("a")) {
        runModeA(opt);
    } else if (opt.mode == QStringLiteral("b")) {
        runModeB(opt, image);
    } else if (opt.mode == QStringLiteral("c")) {
        runModeC(opt, image);
    } else if (opt.mode == QStringLiteral("d")) {
        runModeD(opt);
    } else if (opt.mode == QStringLiteral("e")) {
        runModeE(opt, image);
    } else if (opt.mode == QStringLiteral("qlocale")) {
        runModeQSystemLocale(opt);
    } else if (opt.mode == QStringLiteral("legacy")) {
        runModeLegacy(opt, image);
    } else {
        std::printf("[probe] unknown mode: %s\n", qPrintable(opt.mode));
        return 2;
    }

    const long long ok = g_ok.load();
    const long long fail = g_fail.load();
    std::printf("[probe] done ok=%lld fail=%lld elapsed_ms=%lld\n",
                ok, fail, static_cast<long long>(timer.elapsed()));
    std::fflush(stdout);

    return fail == 0 ? 0 : 1;
}
