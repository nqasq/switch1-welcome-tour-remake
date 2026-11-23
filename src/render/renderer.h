#pragma once
#include <string>
#include <vector>

#include "assets/gltf_loader.h"

#include "render/gl.h"
#include "render/mat4.h"

namespace render {

// Draws everything as lit boxes for now. Our own models can replace the
// cube mesh later; no outside assets are loaded.
class Renderer {
public:
    bool init(std::string* error);
    void shutdown();

    void beginFrame(int width, int height, const Camera& camera);
    // inside = true draws the faces from the inside (used for the room).
    void drawBox(Vec3 center, Vec3 halfExtents, Vec3 color, bool inside = false, bool checker = false);
    // Uploads a loaded model to the GPU; returns its id, or -1 on failure.
    int uploadModel(const assets::MeshData& mesh);
    // Draws a model with the given model matrix (metres in, metres out).
    void drawModel(int id, const Mat4& model);

    // Saves the current back buffer as a .bmp (call before swapping).
    bool saveScreenshot(const char* path, int width, int height);

private:
    struct GpuModel {
        GLuint vao = 0, vbo = 0;
        std::vector<assets::MeshData::Part> parts;
    };
    void setCommonUniforms(const Mat4& model, Vec3 color, float normalSign, bool checker);

    GLuint program_ = 0, vao_ = 0, vbo_ = 0;
    std::vector<GpuModel> models_;
    GLint uMvp_ = -1, uModel_ = -1, uColor_ = -1, uNormalSign_ = -1, uChecker_ = -1;
    Mat4 viewProj_;
};

} // namespace render
