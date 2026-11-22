#pragma once
#include "physics/body.h"

namespace phys {

// A field gives every body an acceleration that depends on where it is
// (and, later, what it is). Gravity is the first one; wind, magnets or a
// "low gravity" exhibit zone can be added as more Field subclasses.
class Field {
public:
    virtual ~Field() = default;
    virtual Vec3 acceleration(const Body& body) const = 0;
};

class GravityField : public Field {
public:
    explicit GravityField(Vec3 g = {0.0f, -9.81f, 0.0f}) : g_(g) {}
    Vec3 acceleration(const Body&) const override { return g_; }
    void setG(Vec3 g) { g_ = g; }
    Vec3 g() const { return g_; }

private:
    Vec3 g_;
};

} // namespace phys
