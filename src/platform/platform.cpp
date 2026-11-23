#include "platform/platform.h"

#include <SDL.h>

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
#endif

namespace platform {

#ifdef __SWITCH__
namespace {
int g_nxlinkSock = -1;
}

void init() {
    romfsInit(); // assets/ is packed into the .nro as romfs
    // When launched with `nxlink -s`, printf output shows up on the PC.
    if (R_SUCCEEDED(socketInitializeDefault())) {
        g_nxlinkSock = nxlinkStdio();
        if (g_nxlinkSock < 0) socketExit();
    }
}

void shutdown() {
    if (g_nxlinkSock >= 0) {
        close(g_nxlinkSock);
        socketExit();
        g_nxlinkSock = -1;
    }
    romfsExit();
}

bool keepRunning() { return appletMainLoop(); }
WindowConfig windowConfig() { return {1280, 720, true}; }
const char* name() { return "Switch"; }
std::string assetPath(const std::string& relative) { return "romfs:/" + relative; }

#else

void init() {}
void shutdown() {}
bool keepRunning() { return true; }
WindowConfig windowConfig() { return {1280, 720, false}; }
const char* name() { return "PC"; }

std::string assetPath(const std::string& relative) {
    std::string base = "./";
    if (char* sdlBase = SDL_GetBasePath()) {
        base = sdlBase;
        SDL_free(sdlBase);
    }
    return base + "assets/" + relative;
}

#endif

} // namespace platform
