#pragma once
#include <vector>

#include "physics/world.h"

// The first exhibit: a "gravity room" with a few boxes to drop and drag.
// Kept free of SDL/OpenGL so the tests can build the same scene.
namespace demo {

struct Scene {
    phys::World world;
    std::vector<phys::Vec3> colors; // one per body
};

phys::Aabb roomBounds();
void populate(Scene& scene);

} // namespace demo
