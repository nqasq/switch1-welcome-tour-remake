#pragma once
#include <cmath>

#include "physics/vec3.h"

namespace render {

using phys::Vec3;

// Column-major 4x4 matrix, the layout OpenGL expects.
struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    float& at(int row, int col) { return m[col * 4 + row]; }
    float at(int row, int col) const { return m[col * 4 + row]; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int rr = 0; rr < 4; ++rr) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) s += at(rr, k) * o.at(k, c);
                r.at(rr, c) = s;
            }
        return r;
    }

    static Mat4 translate(Vec3 t) {
        Mat4 r;
        r.at(0, 3) = t.x; r.at(1, 3) = t.y; r.at(2, 3) = t.z;
        return r;
    }
    static Mat4 scale(Vec3 s) {
        Mat4 r;
        r.at(0, 0) = s.x; r.at(1, 1) = s.y; r.at(2, 2) = s.z;
        return r;
    }
    static Mat4 rotateY(float radians) {
        Mat4 r;
        float c = std::cos(radians), s = std::sin(radians);
        r.at(0, 0) = c;  r.at(0, 2) = s;
        r.at(2, 0) = -s; r.at(2, 2) = c;
        return r;
    }
    static Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
        Mat4 r;
        float f = 1.0f / std::tan(fovY * 0.5f);
        r.at(0, 0) = f / aspect;
        r.at(1, 1) = f;
        r.at(2, 2) = (zFar + zNear) / (zNear - zFar);
        r.at(2, 3) = 2.0f * zFar * zNear / (zNear - zFar);
        r.at(3, 2) = -1.0f;
        r.at(3, 3) = 0.0f;
        return r;
    }
    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f = phys::normalize(center - eye);
        Vec3 s = phys::normalize(phys::cross(f, up));
        Vec3 u = phys::cross(s, f);
        Mat4 r;
        r.at(0, 0) = s.x; r.at(0, 1) = s.y; r.at(0, 2) = s.z;
        r.at(1, 0) = u.x; r.at(1, 1) = u.y; r.at(1, 2) = u.z;
        r.at(2, 0) = -f.x; r.at(2, 1) = -f.y; r.at(2, 2) = -f.z;
        r.at(0, 3) = -phys::dot(s, eye);
        r.at(1, 3) = -phys::dot(u, eye);
        r.at(2, 3) = phys::dot(f, eye);
        return r;
    }
};

// First-person camera. yaw = 0 looks down -Z, positive pitch looks up.
struct Camera {
    Vec3 position{0.0f, 1.6f, 3.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float fovY = 1.15f; // ~66 degrees

    Vec3 forward() const {
        return {std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw)};
    }
    Vec3 flatForward() const { return {std::sin(yaw), 0.0f, -std::cos(yaw)}; }
    Vec3 flatRight() const { return {std::cos(yaw), 0.0f, std::sin(yaw)}; }
    Mat4 view() const { return Mat4::lookAt(position, position + forward(), {0.0f, 1.0f, 0.0f}); }
};

} // namespace render
