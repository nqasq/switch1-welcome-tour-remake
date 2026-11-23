#pragma once
#include <string>
#include <vector>

#include "physics/vec3.h"

namespace assets {

// A glTF model flattened into one triangle list, ready for a single vertex
// buffer. Kept free of OpenGL so the tests can load models too.
struct MeshData {
    struct Part {
        int firstVertex = 0;
        int vertexCount = 0;
        phys::Vec3 color{0.8f, 0.8f, 0.8f}; // display colour (sRGB-ish), from the material
    };

    std::vector<float> vertices; // interleaved: position xyz, normal xyz (model space, metres)
    std::vector<Part> parts;     // one per glTF primitive
    phys::Vec3 boundsMin;
    phys::Vec3 boundsMax;

    int vertexCount() const { return static_cast<int>(vertices.size() / 6); }
};

// Loads a .glb or .gltf file. Node transforms are baked in, indices are
// expanded, and each primitive's base colour becomes a Part colour.
bool loadGltf(const std::string& path, MeshData* out, std::string* error);

} // namespace assets
