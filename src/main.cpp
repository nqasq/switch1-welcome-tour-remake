// Switch1 Welcome Tour Remake: first exhibit, the gravity room.
// PC controls: WASD move, hold right mouse to look, hold left mouse to drag
// the box under the crosshair, R reset, Esc quit.
// Switch controls: left stick move, right stick or touch drag to look,
// hold ZR (or R) to drag, X reset, + quit.
#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "assets/gltf_loader.h"
#include "demo.h"
#include "input/input.h"
#include "platform/platform.h"
#include "render/renderer.h"

namespace {

constexpr float kWalkSpeed = 2.5f;    // metres per second
constexpr float kEyeHeight = 1.6f;
constexpr float kWallMargin = 0.3f;
constexpr float kMaxPitch = 1.45f;

// Optional dev flags:
//   --frames N         quit after N frames (for quick smoke tests)
//   --screenshot FILE  save the last frame as a .bmp before quitting
//   --camera X,Y,Z,YAW,PITCH  start somewhere else (metres, radians)
const char* argValue(int argc, char** argv, const char* flag) {
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], flag) == 0) return argv[i + 1];
    return nullptr;
}

} // namespace

int main(int argc, char** argv) {
    platform::init();
    const char* framesArg = argValue(argc, argv, "--frames");
    const int frameLimit = framesArg ? std::atoi(framesArg) : -1; // -1: run until quit
    const char* screenshotPath = argValue(argc, argv, "--screenshot");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        platform::shutdown();
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    platform::WindowConfig wc = platform::windowConfig();
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (wc.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    SDL_Window* window = SDL_CreateWindow("Switch1 Welcome Tour Remake", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, wc.width, wc.height, flags);
    if (!window) {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        platform::shutdown();
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        platform::shutdown();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    render::Renderer renderer;
    std::string error;
    if (!renderer.init(&error)) {
        std::printf("Renderer init failed: %s\n", error.c_str());
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        platform::shutdown();
        return 1;
    }

    input::Input input;
    input.init();

    demo::Scene scene{phys::World(demo::roomBounds()), {}};
    scene.world.addField(std::make_unique<phys::GravityField>());
    demo::populate(scene);
    const phys::Aabb room = scene.world.room();

    // The Switch 1 model, at real size (1:1) on a pedestal in front of the start point.
    constexpr float kPedestalTop = 1.0f;
    const phys::Vec3 kPedestalPos{0.0f, kPedestalTop * 0.5f, 1.7f};
    int switchModel = -1;
    phys::Vec3 modelBottom;
    {
        assets::MeshData mesh;
        std::string modelError;
        std::string path = platform::assetPath("models/switch1_handheld.glb");
        if (assets::loadGltf(path, &mesh, &modelError)) {
            switchModel = renderer.uploadModel(mesh);
            modelBottom = {(mesh.boundsMin.x + mesh.boundsMax.x) * 0.5f, mesh.boundsMin.y,
                           (mesh.boundsMin.z + mesh.boundsMax.z) * 0.5f};
            std::printf("Loaded %s: %d vertices, %zu parts, %.1f x %.1f x %.1f mm\n", path.c_str(),
                        mesh.vertexCount(), mesh.parts.size(),
                        (mesh.boundsMax.x - mesh.boundsMin.x) * 1000.0f,
                        (mesh.boundsMax.y - mesh.boundsMin.y) * 1000.0f,
                        (mesh.boundsMax.z - mesh.boundsMin.z) * 1000.0f);
        } else {
            std::printf("Model not loaded: %s\n", modelError.c_str());
        }
    }

    render::Camera camera;
    camera.position = {0.0f, kEyeHeight, 2.5f};
    camera.pitch = -0.32f; // look down a little, at the model on its pedestal
    if (const char* cam = argValue(argc, argv, "--camera")) {
        std::sscanf(cam, "%f,%f,%f,%f,%f", &camera.position.x, &camera.position.y, &camera.position.z,
                    &camera.yaw, &camera.pitch);
    }
    float grabDistance = 0.0f;

    std::printf("Gravity room running on %s\n", platform::name());
    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    int frame = 0;

    while (platform::keepRunning()) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - last) / freq);
        last = now;
        dt = std::min(dt, 0.1f);

        input::State in = input.poll(dt);
        if (in.quit) break;
        if (in.resetPressed) demo::populate(scene);

        // Look and walk.
        camera.yaw += in.lookYaw;
        camera.pitch = std::clamp(camera.pitch + in.lookPitch, -kMaxPitch, kMaxPitch);
        phys::Vec3 move = camera.flatForward() * in.moveForward + camera.flatRight() * in.moveRight;
        if (phys::length(move) > 1.0f) move = phys::normalize(move);
        camera.position += move * (kWalkSpeed * dt);
        camera.position.x = std::clamp(camera.position.x, room.min.x + kWallMargin, room.max.x - kWallMargin);
        camera.position.z = std::clamp(camera.position.z, room.min.z + kWallMargin, room.max.z - kWallMargin);

        // Grab whatever is under the crosshair; hold it at the same distance.
        phys::World& world = scene.world;
        if (in.grabHeld) {
            if (world.grabbed() < 0) {
                float hitDistance = 0.0f;
                int hit = world.raycast(camera.position, camera.forward(), &hitDistance);
                if (hit >= 0 && hitDistance < 4.0f) {
                    grabDistance = std::max(hitDistance + world.body(hit).halfExtents.z, 0.8f);
                    world.grab(hit, camera.position + camera.forward() * grabDistance);
                }
            } else {
                world.setGrabTarget(camera.position + camera.forward() * grabDistance);
            }
        } else if (world.grabbed() >= 0) {
            world.release();
        }

        world.update(dt);

        int w = 0, h = 0;
        SDL_GL_GetDrawableSize(window, &w, &h);
        renderer.beginFrame(w, h, camera);
        phys::Vec3 roomCenter = (room.min + room.max) * 0.5f;
        phys::Vec3 roomHalf = (room.max - room.min) * 0.5f;
        renderer.drawBox(roomCenter, roomHalf, {0.78f, 0.76f, 0.72f}, /*inside=*/true, /*checker=*/true);
        for (int i = 0; i < world.bodyCount(); ++i) {
            phys::Vec3 c = scene.colors[i];
            if (i == world.grabbed()) c = c * 1.25f;
            renderer.drawBox(world.body(i).position, world.body(i).halfExtents, c);
        }
        // Pedestal and the 1:1 Switch 1 standing on it, screen toward the start point.
        renderer.drawBox(kPedestalPos, {0.18f, kPedestalTop * 0.5f, 0.12f}, {0.55f, 0.53f, 0.50f});
        if (switchModel >= 0) {
            phys::Vec3 spot{kPedestalPos.x, kPedestalTop, kPedestalPos.z};
            renderer.drawModel(switchModel, render::Mat4::translate(spot - modelBottom));
        }

        // Crosshair: a tiny box just in front of the eye.
        renderer.drawBox(camera.position + camera.forward() * 0.5f, {0.003f, 0.003f, 0.003f}, {1, 1, 1});

        ++frame;
        const bool lastFrame = frameLimit > 0 && frame >= frameLimit;
        if (lastFrame && screenshotPath) {
            if (renderer.saveScreenshot(screenshotPath, w, h))
                std::printf("Saved %s\n", screenshotPath);
            else
                std::printf("Screenshot failed: %s\n", SDL_GetError());
        }

        SDL_GL_SwapWindow(window);

        if (lastFrame) {
            std::printf("Ran %d frames, exiting\n", frame);
            break;
        }
    }

    input.shutdown();
    renderer.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    platform::shutdown();
    return 0;
}
