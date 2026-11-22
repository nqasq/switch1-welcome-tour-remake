#pragma once
#include <string>

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
    // Saves the current back buffer as a .bmp (call before swapping).
    bool saveScreenshot(const char* path, int width, int height);

private:
    GLuint program_ = 0, vao_ = 0, vbo_ = 0;
    GLint uMvp_ = -1, uModel_ = -1, uColor_ = -1, uNormalSign_ = -1, uChecker_ = -1;
    Mat4 viewProj_;
};

} // namespace render
