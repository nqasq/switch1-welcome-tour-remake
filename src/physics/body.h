#pragma once
#include "physics/vec3.h"

namespace phys {

// An axis-aligned box. Rotation is left out on purpose for now:
// the first exhibit only needs things that fall, bounce and stack.
struct Body {
    Vec3 position;
    Vec3 velocity;
    Vec3 halfExtents{0.5f, 0.5f, 0.5f};
    float inverseMass = 1.0f;   // 0 = static, never moves
    float restitution = 0.4f;   // bounciness, 0..1
    float friction = 0.6f;      // how fast sliding stops on a surface
    Vec3 force;                 // accumulated for the current step, cleared after

    bool isStatic() const { return inverseMass == 0.0f; }
    void applyForce(Vec3 f) { force += f; }
};

} // namespace phys
