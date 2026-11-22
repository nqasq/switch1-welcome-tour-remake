// Plain assert-style tests for the physics module. No SDL/OpenGL needed.
#include <cmath>
#include <cstdio>
#include <memory>

#include "demo.h"
#include "physics/world.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

phys::World makeWorld() {
    phys::World w({{-4, 0, -4}, {4, 3, 4}});
    w.addField(std::make_unique<phys::GravityField>());
    return w;
}

void runSeconds(phys::World& w, float seconds) {
    int steps = static_cast<int>(seconds / phys::World::kFixedDt);
    for (int i = 0; i < steps; ++i) w.step();
}

void testGravityField() {
    std::printf("gravity field\n");
    phys::GravityField g;
    phys::Body b;
    CHECK(std::fabs(g.acceleration(b).y + 9.81f) < 1e-6f);
    g.setG({0, -1.62f, 0}); // the Moon, for a future exhibit
    CHECK(std::fabs(g.acceleration(b).y + 1.62f) < 1e-6f);
}

void testFreeFall() {
    std::printf("free fall matches 1/2 g t^2\n");
    phys::World w = makeWorld();
    phys::Body b;
    b.position = {0, 2.9f, 0};
    b.halfExtents = {0.05f, 0.05f, 0.05f};
    int id = w.addBody(b);
    runSeconds(w, 0.5f);
    float expectedDrop = 0.5f * 9.81f * 0.25f; // ~1.226 m
    float drop = 2.9f - w.body(id).position.y;
    CHECK(std::fabs(drop - expectedDrop) < 0.03f);
}

void testFallsAndRests() {
    std::printf("box falls, bounces and comes to rest on the floor\n");
    phys::World w = makeWorld();
    phys::Body b;
    b.position = {0, 2.5f, 0};
    int id = w.addBody(b);

    bool bounced = false;
    for (int i = 0; i < 600; ++i) { // 5 seconds
        w.step();
        if (w.body(id).velocity.y > 0.5f) bounced = true;
    }
    CHECK(bounced);
    CHECK(std::fabs(w.body(id).position.y - 0.5f) < 0.01f);
    CHECK(phys::length(w.body(id).velocity) < 0.05f);
}

void testStacking() {
    std::printf("two boxes stack without sinking into each other\n");
    phys::World w = makeWorld();
    phys::Body bottom, top;
    bottom.position = {0, 0.5f, 0};
    top.position = {0.1f, 2.0f, 0};
    int a = w.addBody(bottom);
    int c = w.addBody(top);
    runSeconds(w, 4.0f);
    float gap = w.body(c).position.y - w.body(a).position.y;
    CHECK(gap > 0.97f && gap < 1.03f);
    CHECK(std::fabs(w.body(a).position.y - 0.5f) < 0.03f);
}

void testWalls() {
    std::printf("boxes stay inside the room\n");
    phys::World w = makeWorld();
    phys::Body b;
    b.position = {0, 1.0f, 0};
    b.velocity = {30.0f, 10.0f, -25.0f};
    int id = w.addBody(b);
    for (int i = 0; i < 1200; ++i) {
        w.step();
        const phys::Vec3 p = w.body(id).position;
        if (p.x < -3.51f || p.x > 3.51f || p.y < 0.49f || p.y > 2.51f || p.z < -3.51f || p.z > 3.51f) {
            CHECK(false && "left the room");
            break;
        }
    }
}

void testDrag() {
    std::printf("dragging pulls a box to the target and release lets it fall\n");
    phys::World w = makeWorld();
    phys::Body b;
    b.position = {-2, 0.5f, -2};
    int id = w.addBody(b);
    phys::Vec3 target{1.0f, 2.0f, 0.5f};
    w.grab(id, target);
    runSeconds(w, 2.0f);
    CHECK(phys::length(w.body(id).position - target) < 0.05f);
    w.release();
    CHECK(w.grabbed() == -1);
    runSeconds(w, 3.0f);
    CHECK(std::fabs(w.body(id).position.y - 0.5f) < 0.02f);
}

void testRaycast() {
    std::printf("raycast picks the nearest box\n");
    phys::World w = makeWorld();
    phys::Body near, far;
    near.position = {0, 1, -2};
    far.position = {0, 1, -3.4f};
    w.addBody(far);
    int nearId = w.addBody(near);
    float dist = 0.0f;
    int hit = w.raycast({0, 1, 2}, {0, 0, -1}, &dist);
    CHECK(hit == nearId);
    CHECK(std::fabs(dist - 3.5f) < 1e-4f);
    CHECK(w.raycast({0, 1, 2}, {0, 1, 0}, nullptr) == -1);
}

void testFixedStepAccumulator() {
    std::printf("update() runs fixed 1/120 s steps and caps slow frames\n");
    phys::World w = makeWorld();
    CHECK(w.update(1.0f / 60.0f) == 2);
    CHECK(w.update(0.004f) == 0);
    CHECK(w.update(0.005f) == 1);
    CHECK(w.update(1.0f) == phys::World::kMaxStepsPerUpdate);
}

void testDemoSceneSettles() {
    std::printf("demo scene settles inside the room\n");
    demo::Scene scene{phys::World(demo::roomBounds()), {}};
    scene.world.addField(std::make_unique<phys::GravityField>());
    demo::populate(scene);
    CHECK(scene.world.bodyCount() == 4);
    CHECK(scene.colors.size() == 4u);
    runSeconds(scene.world, 6.0f);
    for (int i = 0; i < scene.world.bodyCount(); ++i) {
        const phys::Body& b = scene.world.body(i);
        CHECK(phys::length(b.velocity) < 0.1f);
        CHECK(b.position.y - b.halfExtents.y > -0.01f);
    }
}

} // namespace

int main() {
    testGravityField();
    testFreeFall();
    testFallsAndRests();
    testStacking();
    testWalls();
    testDrag();
    testRaycast();
    testFixedStepAccumulator();
    testDemoSceneSettles();
    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("all physics tests passed\n");
    return 0;
}
