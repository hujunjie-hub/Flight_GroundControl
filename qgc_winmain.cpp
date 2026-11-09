/* Flight custom build: MinGW selected the GUI startup object for the QGC
 * executable, so provide the WinMain shim that forwards to the normal main().
 * The command line is rebuilt explicitly because the GUI CRT does not populate
 * __argc/__argv for this static MinGW build.
 */
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>

extern "C" int main(int argc, char **argv);

extern "C" int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                             LPSTR command_line, int show)
{
    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show;

    int argc = 0;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!wide || argc < 1) {
        char *fallback[1] = {nullptr};
        return main(0, fallback);
    }
    char **argv = static_cast<char **>(calloc(static_cast<size_t>(argc) + 1, sizeof(char *)));
    if (!argv)
        return 1;
    for (int i = 0; i < argc; ++i) {
        const int length = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
        argv[i] = static_cast<char *>(malloc(static_cast<size_t>(length > 0 ? length : 1)));
        if (argv[i])
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, argv[i], length, nullptr, nullptr);
    }
    const int result = main(argc, argv);
    LocalFree(wide);
    return result;
}
