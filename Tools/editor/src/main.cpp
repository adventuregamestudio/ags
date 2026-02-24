// AGS Editor ImGui - Cross-platform AGS Editor using Dear ImGui
// Entry point for all platforms (Linux, Windows, macOS)
// Uses SDL2 + SDL_Renderer as ImGui backend.

#include "app.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    ::SetProcessDPIAware();
#endif

    AGSEditorApp app;
    if (!app.Init(argc, argv))
    {
        fprintf(stderr, "Failed to initialize AGS Editor\n");
        return 1;
    }

    if (app.IsHeadless())
    {
        int result = app.RunHeadless();
        app.Shutdown();
        return result;
    }

    app.Run();
    app.Shutdown();

    return app.GetExitCode();
}
