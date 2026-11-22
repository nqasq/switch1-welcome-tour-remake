#include "platform/platform.h"

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
}

bool keepRunning() { return appletMainLoop(); }
WindowConfig windowConfig() { return {1280, 720, true}; }
const char* name() { return "Switch"; }

#else

void init() {}
void shutdown() {}
bool keepRunning() { return true; }
WindowConfig windowConfig() { return {1280, 720, false}; }
const char* name() { return "PC"; }

#endif

} // namespace platform
