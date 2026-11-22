#pragma once
#include <SDL.h>

namespace input {

// What the game wants to know, whatever the device.
struct State {
    float moveForward = 0.0f; // -1..1
    float moveRight = 0.0f;   // -1..1
    float lookYaw = 0.0f;     // radians to turn this frame
    float lookPitch = 0.0f;
    bool grabHeld = false;
    bool resetPressed = false;
    bool quit = false;
};

class Input {
public:
    void init();
    void shutdown();
    // Drains SDL events and reads devices. frameSeconds scales stick turning.
    State poll(float frameSeconds);

private:
    SDL_Joystick* joystick_ = nullptr;
    bool lookDragging_ = false;
};

} // namespace input
