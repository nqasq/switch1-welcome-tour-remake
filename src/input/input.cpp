#include "input/input.h"

#include <cmath>

namespace input {

namespace {
constexpr float kMouseSensitivity = 0.0035f; // radians per pixel
constexpr float kStickTurnSpeed = 2.4f;      // radians per second at full tilt
constexpr float kTouchSensitivity = 3.0f;    // radians per full screen width
constexpr float kDeadZone = 0.18f;

#ifdef __SWITCH__
// SDL2 joystick button numbers on the Switch port. Not tested on hardware yet.
enum SwitchButton { kA = 0, kB = 1, kX = 2, kY = 3, kL = 6, kR = 7, kZL = 8, kZR = 9, kPlus = 10 };
#endif

float axis(SDL_Joystick* js, int index) {
    if (!js) return 0.0f;
    float v = SDL_JoystickGetAxis(js, index) / 32767.0f;
    return std::fabs(v) < kDeadZone ? 0.0f : v;
}
} // namespace

void Input::init() {
    if (SDL_NumJoysticks() > 0) joystick_ = SDL_JoystickOpen(0);
}

void Input::shutdown() {
    if (joystick_) SDL_JoystickClose(joystick_);
    joystick_ = nullptr;
}

State Input::poll(float frameSeconds) {
    State s;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            s.quit = true;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE) s.quit = true;
            if (e.key.keysym.sym == SDLK_r && !e.key.repeat) s.resetPressed = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button == SDL_BUTTON_RIGHT) {
                lookDragging_ = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_RIGHT) {
                lookDragging_ = false;
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            break;
        case SDL_MOUSEMOTION:
            if (lookDragging_) {
                s.lookYaw += e.motion.xrel * kMouseSensitivity;
                s.lookPitch -= e.motion.yrel * kMouseSensitivity;
            }
            break;
        case SDL_FINGERMOTION:
            // Touch screen on Switch: drag a finger to look around.
            s.lookYaw += e.tfinger.dx * kTouchSensitivity;
            s.lookPitch -= e.tfinger.dy * kTouchSensitivity;
            break;
        case SDL_JOYDEVICEADDED:
            if (!joystick_) joystick_ = SDL_JoystickOpen(e.jdevice.which);
            break;
#ifdef __SWITCH__
        case SDL_JOYBUTTONDOWN:
            if (e.jbutton.button == kPlus) s.quit = true;
            if (e.jbutton.button == kX) s.resetPressed = true;
            break;
#endif
        default:
            break;
        }
    }

    // Keyboard (PC).
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_W]) s.moveForward += 1.0f;
    if (keys[SDL_SCANCODE_S]) s.moveForward -= 1.0f;
    if (keys[SDL_SCANCODE_D]) s.moveRight += 1.0f;
    if (keys[SDL_SCANCODE_A]) s.moveRight -= 1.0f;
    if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) s.grabHeld = true;

    // Sticks: left moves, right looks (Switch Joy-Con / Pro Controller, or a PC gamepad).
    s.moveForward -= axis(joystick_, 1);
    s.moveRight += axis(joystick_, 0);
    s.lookYaw += axis(joystick_, 2) * kStickTurnSpeed * frameSeconds;
    s.lookPitch -= axis(joystick_, 3) * kStickTurnSpeed * frameSeconds;

#ifdef __SWITCH__
    if (joystick_ && (SDL_JoystickGetButton(joystick_, kZR) || SDL_JoystickGetButton(joystick_, kR)))
        s.grabHeld = true;
#endif

    if (s.moveForward > 1.0f) s.moveForward = 1.0f;
    if (s.moveForward < -1.0f) s.moveForward = -1.0f;
    if (s.moveRight > 1.0f) s.moveRight = 1.0f;
    if (s.moveRight < -1.0f) s.moveRight = -1.0f;
    return s;
}

} // namespace input
