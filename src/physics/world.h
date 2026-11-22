#pragma once
#include <memory>
#include <vector>

#include "physics/body.h"
#include "physics/field.h"

namespace phys {

struct Aabb {
    Vec3 min;
    Vec3 max;
};

class World {
public:
    static constexpr float kFixedDt = 1.0f / 120.0f;
    static constexpr int kMaxStepsPerUpdate = 8;

    explicit World(Aabb room);

    int addBody(const Body& body);
    Body& body(int id) { return bodies_[id]; }
    const Body& body(int id) const { return bodies_[id]; }
    int bodyCount() const { return static_cast<int>(bodies_.size()); }
    void clearBodies();

    void addField(std::unique_ptr<Field> field);
    const Aabb& room() const { return room_; }

    // Dragging: a damped spring pulls the grabbed body toward a target point.
    void grab(int id, Vec3 target);
    void setGrabTarget(Vec3 target) { grabTarget_ = target; }
    void release() { grabbed_ = -1; }
    int grabbed() const { return grabbed_; }

    // One fixed step of kFixedDt seconds.
    void step();
    // Feeds real frame time into an accumulator and runs as many fixed steps
    // as fit (capped, so a slow frame can't spiral). Returns the step count.
    int update(float frameSeconds);

    // Returns the id of the nearest body hit by the ray, or -1.
    int raycast(Vec3 origin, Vec3 dir, float* distance) const;

private:
    void integrate(float dt);
    void collideWithRoom(Body& b, float dt);
    void collideBodies();

    std::vector<Body> bodies_;
    std::vector<std::unique_ptr<Field>> fields_;
    Aabb room_;
    float accumulator_ = 0.0f;
    int grabbed_ = -1;
    Vec3 grabTarget_;
};

} // namespace phys
