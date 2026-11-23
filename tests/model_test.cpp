// Loads the exported Switch 1 model and checks it matches the official sizes.
#include <cmath>
#include <cstdio>
#include <string>

#include "assets/gltf_loader.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

bool near(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

} // namespace

int main() {
    const std::string path = std::string(TOUR_ASSET_DIR) + "/models/switch1_handheld.glb";
    assets::MeshData mesh;
    std::string error;
    std::printf("load %s\n", path.c_str());
    CHECK(assets::loadGltf(path, &mesh, &error));
    if (!error.empty()) std::printf("  %s\n", error.c_str());
    if (g_failures) return 1;

    const float w = (mesh.boundsMax.x - mesh.boundsMin.x) * 1000.0f;
    const float h = (mesh.boundsMax.y - mesh.boundsMin.y) * 1000.0f;
    const float d = (mesh.boundsMax.z - mesh.boundsMin.z) * 1000.0f;
    std::printf("size %.2f x %.2f x %.2f mm, %d vertices, %zu parts\n", w, h, d, mesh.vertexCount(),
                mesh.parts.size());

    std::printf("official sizes: 239 mm wide, 102 mm tall, 28.4 mm thick incl. sticks and ZL/ZR\n");
    CHECK(near(w, 239.0f, 0.2f));
    CHECK(near(h, 102.0f, 0.6f)); // shoulder buttons may poke out by 0.5 mm
    CHECK(near(d, 28.4f, 0.2f));

    std::printf("every part has triangles and a colour\n");
    for (const auto& p : mesh.parts) {
        CHECK(p.vertexCount > 0 && p.vertexCount % 3 == 0);
        CHECK(p.color.x >= 0.0f && p.color.x <= 1.0f);
    }

    std::printf("front faces +Z: stick caps point at the viewer\n");
    // The stick cap tops are the front-most surface. Triangles lying flat on it
    // must have normals toward +Z, which also checks that rotated nodes (the
    // cylinders) get their normals rotated.
    int capTris = 0, wrong = 0;
    const float front = mesh.boundsMax.z - 0.00005f;
    for (int t = 0; t + 2 < mesh.vertexCount(); t += 3) {
        const float* v = &mesh.vertices[t * 6];
        if (v[2] > front && v[8] > front && v[14] > front) {
            ++capTris;
            for (int k = 0; k < 3; ++k)
                if (v[k * 6 + 5] < 0.99f) ++wrong;
        }
    }
    CHECK(capTris > 0);
    CHECK(wrong == 0);

    std::printf("blue Joy-Con on the viewer's left (-X), red on the right (+X)\n");
    float blueX = 0.0f, redX = 0.0f;
    int blueN = 0, redN = 0;
    for (const auto& p : mesh.parts) {
        bool blue = p.color.z > 0.6f && p.color.x < 0.3f;
        bool red = p.color.x > 0.8f && p.color.z < 0.5f;
        for (int k = 0; k < p.vertexCount; ++k) {
            float x = mesh.vertices[(p.firstVertex + k) * 6];
            if (blue) { blueX += x; ++blueN; }
            if (red) { redX += x; ++redN; }
        }
    }
    CHECK(blueN > 0 && redN > 0);
    if (blueN && redN) CHECK(blueX / blueN < -0.08f && redX / redN > 0.08f);

    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("all model tests passed\n");
    return 0;
}
