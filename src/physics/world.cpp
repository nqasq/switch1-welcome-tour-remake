#include "physics/world.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace phys {

namespace {
constexpr float kGrabStiffness = 120.0f;   // spring k (per unit mass)
constexpr float kRestSpeed = 0.05f;        // below this a bounce just stops
constexpr int kSolverIterations = 4;
} // namespace

World::World(Aabb room) : room_(room) {}

int World::addBody(const Body& body) {
    bodies_.push_back(body);
    return static_cast<int>(bodies_.size()) - 1;
}

void World::clearBodies() {
    bodies_.clear();
    grabbed_ = -1;
}

void World::addField(std::unique_ptr<Field> field) { fields_.push_back(std::move(field)); }

void World::grab(int id, Vec3 target) {
    if (id < 0 || id >= bodyCount() || bodies_[id].isStatic()) return;
    grabbed_ = id;
    grabTarget_ = target;
}

void World::step() {
    integrate(kFixedDt);
    for (int i = 0; i < kSolverIterations; ++i) collideBodies();
    for (Body& b : bodies_) collideWithRoom(b, kFixedDt);
}

int World::update(float frameSeconds) {
    accumulator_ += std::max(0.0f, frameSeconds);
    int steps = 0;
    while (accumulator_ >= kFixedDt && steps < kMaxStepsPerUpdate) {
        step();
        accumulator_ -= kFixedDt;
        ++steps;
    }
    if (steps == kMaxStepsPerUpdate) accumulator_ = 0.0f; // drop the backlog
    return steps;
}

void World::integrate(float dt) {
    for (int i = 0; i < bodyCount(); ++i) {
        Body& b = bodies_[i];
        if (b.isStatic()) { b.force = {}; continue; }

        Vec3 accel = b.force * b.inverseMass;
        if (i == grabbed_) {
            // Critically damped spring: holds the box without gravity sag
            // fighting it, so it ignores fields while held.
            float damping = 2.0f * std::sqrt(kGrabStiffness);
            accel += (grabTarget_ - b.position) * kGrabStiffness - b.velocity * damping;
        } else {
            for (const auto& f : fields_) accel += f->acceleration(b);
        }

        // Semi-implicit Euler: update velocity first, then position.
        b.velocity += accel * dt;
        b.position += b.velocity * dt;
        b.force = {};
    }
}

void World::collideWithRoom(Body& b, float dt) {
    if (b.isStatic()) return;
    for (int axis = 0; axis < 3; ++axis) {
        float lo = room_.min[axis] + b.halfExtents[axis];
        float hi = room_.max[axis] - b.halfExtents[axis];
        bool hitLow = b.position[axis] < lo;
        bool hitHigh = b.position[axis] > hi;
        if (!hitLow && !hitHigh) continue;

        b.position[axis] = hitLow ? lo : hi;
        float v = b.velocity[axis];
        if ((hitLow && v < 0.0f) || (hitHigh && v > 0.0f)) {
            v = -v * b.restitution;
            if (std::fabs(v) < kRestSpeed) v = 0.0f;
            b.velocity[axis] = v;
        }

        // Friction on the two other axes while touching this wall.
        float keep = std::max(0.0f, 1.0f - b.friction * 10.0f * dt);
        for (int t = 0; t < 3; ++t)
            if (t != axis) b.velocity[t] *= keep;
    }
}

void World::collideBodies() {
    for (int i = 0; i < bodyCount(); ++i) {
        for (int j = i + 1; j < bodyCount(); ++j) {
            Body& a = bodies_[i];
            Body& c = bodies_[j];
            float invSum = a.inverseMass + c.inverseMass;
            if (invSum == 0.0f) continue;

            Vec3 d = c.position - a.position;
            int axis = -1;
            float minOverlap = std::numeric_limits<float>::max();
            for (int k = 0; k < 3; ++k) {
                float overlap = a.halfExtents[k] + c.halfExtents[k] - std::fabs(d[k]);
                if (overlap <= 0.0f) { axis = -1; break; }
                if (overlap < minOverlap) { minOverlap = overlap; axis = k; }
            }
            if (axis < 0) continue;

            Vec3 n;
            n[axis] = d[axis] >= 0.0f ? 1.0f : -1.0f;

            // Push apart in proportion to inverse mass.
            Vec3 correction = n * (minOverlap / invSum);
            a.position -= correction * a.inverseMass;
            c.position += correction * c.inverseMass;

            // Bounce impulse along the contact normal.
            float vn = dot(c.velocity - a.velocity, n);
            if (vn < 0.0f) {
                float e = std::min(a.restitution, c.restitution);
                if (-vn < kRestSpeed) e = 0.0f;
                float jImpulse = -(1.0f + e) * vn / invSum;
                a.velocity -= n * (jImpulse * a.inverseMass);
                c.velocity += n * (jImpulse * c.inverseMass);
            }
        }
    }
}

int World::raycast(Vec3 origin, Vec3 dir, float* distance) const {
    int best = -1;
    float bestT = std::numeric_limits<float>::max();
    for (int i = 0; i < bodyCount(); ++i) {
        const Body& b = bodies_[i];
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();
        bool hit = true;
        for (int k = 0; k < 3 && hit; ++k) {
            float lo = b.position[k] - b.halfExtents[k];
            float hi = b.position[k] + b.halfExtents[k];
            if (std::fabs(dir[k]) < 1e-8f) {
                if (origin[k] < lo || origin[k] > hi) hit = false;
                continue;
            }
            float t1 = (lo - origin[k]) / dir[k];
            float t2 = (hi - origin[k]) / dir[k];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) hit = false;
        }
        if (hit && tMin < bestT) { bestT = tMin; best = i; }
    }
    if (best >= 0 && distance) *distance = bestT;
    return best;
}

} // namespace phys
