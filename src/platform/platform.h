#pragma once
// Everything that differs between the PC build and the Switch build lives
// behind this header, so the rest of the code has no #ifdef __SWITCH__.

#include <string>

namespace platform {

struct WindowConfig {
    int width;
    int height;
    bool fullscreen;
};

// Console/services setup before SDL starts (romfs, nxlink logging on Switch).
void init();
void shutdown();

// false once the system asks the app to quit (HOME menu close on Switch).
bool keepRunning();

WindowConfig windowConfig();

// Full path of a file under assets/ (PC: next to the executable;
// Switch: inside the .nro's romfs).
std::string assetPath(const std::string& relative);
const char* name();

} // namespace platform
