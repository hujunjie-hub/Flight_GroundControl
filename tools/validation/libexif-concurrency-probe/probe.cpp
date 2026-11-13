// Standalone libexif concurrency probe (FGC-0029). No Qt, no QGroundControl.
//
// Usage: exif_probe <image> <threads> <iterations> [shared|perthread]
//   shared    : every thread parses the same byte buffer (matches QGC, which shares a
//               read-only QByteArray across worker threads)
//   perthread : each thread parses its own copy of the bytes (isolates "shared input")
//
// Exit code: 0 on success, 1 on a failed invariant, and the process aborts with the
// Windows heap-corruption exception (0xC0000374) if libexif corrupts the heap.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "libexif/exif-data.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

bool readFile(const char *path, std::vector<unsigned char> &out)
{
    FILE *f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return false;
    }
    out.resize(static_cast<size_t>(size));
    const size_t read = fread(out.data(), 1, out.size(), f);
    fclose(f);
    return read == out.size();
}

int parseOnce(const unsigned char *data, unsigned int size, bool *ok)
{
    ExifData *exif = exif_data_new_from_data(data, size);
    if (!exif) {
        *ok = false;
        return 0;
    }

    ExifEntry *entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_DATE_TIME_DIGITIZED);
    int length = 0;
    if (entry) {
        char value[256];
        exif_entry_get_value(entry, value, sizeof(value));
        length = static_cast<int>(strnlen(value, sizeof(value)));
        // Touch the entry fields the way ExifUtility::readString does.
        if (entry->size != 0 && entry->components != 0) {
            length += static_cast<int>(entry->size & 1);
        }
    }
    exif_data_unref(exif);
    *ok = true;
    return length;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::printf("usage: exif_probe <image> <threads> <iterations> [shared|perthread]\n");
        return 2;
    }
    const int threadCount = std::atoi(argv[2]);
    const int iterations = std::atoi(argv[3]);
    const std::string mode = (argc >= 5) ? argv[4] : "shared";

    std::vector<unsigned char> shared;
    if (!readFile(argv[1], shared)) {
        std::printf("cannot read %s\n", argv[1]);
        return 2;
    }
    std::printf("image=%s bytes=%zu threads=%d iterations=%d mode=%s\n", argv[1], shared.size(),
                threadCount, iterations, mode.c_str());

#ifdef _WIN32
    // Report the heap integrity of the *main* heap after the run as a cheap oracle.
    const HANDLE heap = GetProcessHeap();
#endif

    std::vector<int> parsedPerThread(static_cast<size_t>(threadCount), 0);
    std::vector<int> failures(static_cast<size_t>(threadCount), 0);
    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(threadCount));

    const auto started = std::chrono::steady_clock::now();
    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            std::vector<unsigned char> local;
            if (mode == "perthread") {
                local = shared;
            }
            const unsigned char *data = (mode == "perthread") ? local.data() : shared.data();
            const unsigned int size = static_cast<unsigned int>((mode == "perthread") ? local.size() : shared.size());
            int parsed = 0;
            int failed = 0;
            for (int i = 0; i < iterations; ++i) {
                bool ok = false;
                const int length = parseOnce(data, size, &ok);
                if (ok) {
                    ++parsed;
                    if (length < 0) {
                        ++failed;
                    }
                } else {
                    ++failed;
                }
            }
            parsedPerThread[static_cast<size_t>(t)] = parsed;
            failures[static_cast<size_t>(t)] = failed;
        });
    }
    for (std::thread &worker : workers) {
        worker.join();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - started).count();

    long total = 0;
    long totalFailures = 0;
    for (int t = 0; t < threadCount; ++t) {
        total += parsedPerThread[static_cast<size_t>(t)];
        totalFailures += failures[static_cast<size_t>(t)];
    }

    std::printf("parsed=%ld failed=%ld elapsedMs=%lld\n", total, totalFailures,
                static_cast<long long>(elapsed));

#ifdef _WIN32
    if (!HeapValidate(heap, 0, nullptr)) {
        std::printf("HEAP INVALID after run\n");
        return 1;
    }
    std::printf("heap=valid\n");
#endif
    return (total > 0 && totalFailures == 0) ? 0 : 1;
}
