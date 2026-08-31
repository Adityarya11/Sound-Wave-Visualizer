// Sound Wave Visualizer - a per-pixel-transparent, always-on-top audio
// overlay driven by WASAPI loopback.
//
// miniaudio is a header-only library: this translation unit is the one place
// its implementation is compiled. Defining the macro anywhere else produces
// duplicate symbols; omitting it entirely produces unresolved ones.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstring>

#include <windows.h>

#include "core/app.hpp"

namespace
{
void showHelp()
{
    MessageBoxW(nullptr,
                L"Sound Wave Visualizer\n\n"
                L"Runs as a transparent, always-on-top overlay and reacts to\n"
                L"whatever Windows is playing. It fades away when audio stops.\n\n"
                L"Global hotkeys (work from any application):\n"
                L"  Ctrl+Alt+V    lock / unlock for moving\n"
                L"  Ctrl+Alt+B    switch orb <-> bars\n"
                L"  Ctrl+Alt+Up   larger\n"
                L"  Ctrl+Alt+Down smaller\n"
                L"  Ctrl+Alt+Q    quit\n\n"
                L"While unlocked:\n"
                L"  drag           move it\n"
                L"  scroll wheel   resize\n"
                L"  Esc / right-click  lock again\n\n"
                L"Position, size and mode are saved to\n"
                L"%APPDATA%\\Visualizer\\config.json.\n"
                L"Edit aurora.frag next to the exe to retune the colours.",
                L"Visualizer", MB_OK | MB_ICONINFORMATION);
}

bool isHelpFlag(const char *arg)
{
    return std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0 ||
           std::strcmp(arg, "/?") == 0;
}
} // namespace

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (isHelpFlag(argv[i]))
        {
            showHelp();
            return 0;
        }
    }

    // A second copy would fight the first one for the topmost slot and double
    // the WASAPI load for no benefit.
    HANDLE instanceLock = CreateMutexW(nullptr, TRUE, L"Local\\SoundWaveVisualizerSingleton");
    if (instanceLock != nullptr && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(instanceLock);
        return 0;
    }

    App app;
    if (!app.init())
    {
        MessageBoxW(nullptr,
                    L"Could not create the overlay window.\n"
                    L"A GPU with OpenGL support is required.",
                    L"Visualizer", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int result = app.run();

    if (instanceLock != nullptr)
    {
        ReleaseMutex(instanceLock);
        CloseHandle(instanceLock);
    }

    return result;
}
