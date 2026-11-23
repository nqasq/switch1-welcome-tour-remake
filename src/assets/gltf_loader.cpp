#include "assets/gltf_loader.h"

#include <cgltf.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace assets {

namespace {

// Blender writes linear base colours; the renderer uses display colours.
float linearToDisplay(float c) { return std::pow(std::clamp(c, 0.0f, 1.0f), 1.0f / 2.2f); }

phys::Vec3 transformPoint(const float m[16], const float p[3]) {
    return {m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
            m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
            m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14]};
}

// Normal matrix = inverse transpose of the upper 3x3. The cofactor matrix is
// that times det, and we normalise afterwards, so only det's sign matters.
void normalMatrix(const float m[16], float out[9]) {
    const float a = m[0], b = m[4], c = m[8];
    const float d = m[1], e = m[5], f = m[9];
    const float g = m[2], h = m[6], i = m[10];
    const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    const float s = det < 0.0f ? -1.0f : 1.0f;
    // Row-major 3x3 of the cofactors.
    out[0] = s * (e * i - f * h); out[1] = s * -(d * i - f * g); out[2] = s * (d * h - e * g);
    out[3] = s * -(b * i - c * h); out[4] = s * (a * i - c * g); out[5] = s * -(a * h - b * g);
    out[6] = s * (b * f - c * e); out[7] = s * -(a * f - c * d); out[8] = s * (a * e - b * d);
}

phys::Vec3 transformNormal(const float n3[9], const float v[3]) {
    // n3 holds the cofactor matrix C (row-major); inverse transpose times v is C * v.
    phys::Vec3 r{n3[0] * v[0] + n3[1] * v[1] + n3[2] * v[2],
                 n3[3] * v[0] + n3[4] * v[1] + n3[5] * v[2],
                 n3[6] * v[0] + n3[7] * v[1] + n3[8] * v[2]};
    return phys::normalize(r);
}

const cgltf_accessor* findAttribute(const cgltf_primitive& prim, cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < prim.attributes_count; ++i)
        if (prim.attributes[i].type == type) return prim.attributes[i].data;
    return nullptr;
}

} // namespace

bool loadGltf(const std::string& path, MeshData* out, std::string* error) {
    auto fail = [&](const std::string& msg) {
        if (error) *error = path + ": " + msg;
        return false;
    };

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
        return fail("could not parse glTF");
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        return fail("could not load glTF buffers");
    }

    MeshData mesh;
    const float inf = std::numeric_limits<float>::max();
    mesh.boundsMin = {inf, inf, inf};
    mesh.boundsMax = {-inf, -inf, -inf};

    for (cgltf_size ni = 0; ni < data->nodes_count; ++ni) {
        const cgltf_node& node = data->nodes[ni];
        if (!node.mesh) continue;
        float world[16];
        cgltf_node_transform_world(&node, world);
        float nmat[9];
        normalMatrix(world, nmat);

        for (cgltf_size pi = 0; pi < node.mesh->primitives_count; ++pi) {
            const cgltf_primitive& prim = node.mesh->primitives[pi];
            if (prim.type != cgltf_primitive_type_triangles) continue;
            const cgltf_accessor* pos = findAttribute(prim, cgltf_attribute_type_position);
            const cgltf_accessor* nrm = findAttribute(prim, cgltf_attribute_type_normal);
            if (!pos) continue;

            MeshData::Part part;
            part.firstVertex = mesh.vertexCount();
            if (prim.material && prim.material->has_pbr_metallic_roughness) {
                const float* c = prim.material->pbr_metallic_roughness.base_color_factor;
                part.color = {linearToDisplay(c[0]), linearToDisplay(c[1]), linearToDisplay(c[2])};
            }

            const cgltf_size count = prim.indices ? prim.indices->count : pos->count;
            for (cgltf_size k = 0; k < count; ++k) {
                const cgltf_size v = prim.indices ? cgltf_accessor_read_index(prim.indices, k) : k;
                float p[3] = {0, 0, 0};
                float n[3] = {0, 1, 0};
                cgltf_accessor_read_float(pos, v, p, 3);
                if (nrm) cgltf_accessor_read_float(nrm, v, n, 3);

                phys::Vec3 wp = transformPoint(world, p);
                phys::Vec3 wn = transformNormal(nmat, n);
                mesh.vertices.insert(mesh.vertices.end(), {wp.x, wp.y, wp.z, wn.x, wn.y, wn.z});
                for (int a = 0; a < 3; ++a) {
                    mesh.boundsMin[a] = std::min(mesh.boundsMin[a], wp[a]);
                    mesh.boundsMax[a] = std::max(mesh.boundsMax[a], wp[a]);
                }
            }
            part.vertexCount = mesh.vertexCount() - part.firstVertex;
            if (part.vertexCount > 0) mesh.parts.push_back(part);
        }
    }
    cgltf_free(data);

    if (mesh.parts.empty()) return fail("no triangle meshes found");
    *out = std::move(mesh);
    return true;
}

} // namespace assets
