#include "demo.h"

#include <memory>

namespace demo {

phys::Aabb roomBounds() { return {{-4.0f, 0.0f, -4.0f}, {4.0f, 3.0f, 4.0f}}; }

void populate(Scene& scene) {
    scene.world.clearBodies();
    scene.colors.clear();

    struct Spec { phys::Vec3 pos, half, color; float restitution; };
    const Spec specs[] = {
        {{-1.2f, 2.2f, -1.5f}, {0.30f, 0.30f, 0.30f}, {0.90f, 0.30f, 0.25f}, 0.55f}, // red, bouncy
        {{ 0.0f, 2.6f, -1.8f}, {0.40f, 0.25f, 0.40f}, {0.25f, 0.60f, 0.95f}, 0.30f}, // blue, flat
        {{ 1.3f, 1.8f, -1.2f}, {0.20f, 0.45f, 0.20f}, {0.95f, 0.80f, 0.25f}, 0.20f}, // yellow, tall
        {{ 0.1f, 1.2f, -1.8f}, {0.35f, 0.35f, 0.35f}, {0.35f, 0.85f, 0.45f}, 0.40f}, // green, under blue
    };
    for (const Spec& s : specs) {
        phys::Body b;
        b.position = s.pos;
        b.halfExtents = s.half;
        b.restitution = s.restitution;
        float volume = 8.0f * s.half.x * s.half.y * s.half.z;
        b.inverseMass = 1.0f / (volume * 400.0f); // like light wood
        scene.world.addBody(b);
        scene.colors.push_back(s.color);
    }
}

} // namespace demo
