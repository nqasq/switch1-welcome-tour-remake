#include "render/renderer.h"

#include <SDL.h>

#include <algorithm>
#include <vector>

namespace render {

namespace {

const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uMvp;
uniform mat4 uModel;
uniform float uNormalSign;
out vec3 vNormal;
out vec3 vWorld;
void main() {
    // mat3(uModel) is enough here: boxes are only scaled along their own axes,
    // and models are placed with rotation plus uniform scale.
    vNormal = mat3(uModel) * aNormal * uNormalSign;
    vWorld = (uModel * vec4(aPos, 1.0)).xyz;
    gl_Position = uMvp * vec4(aPos, 1.0);
}
)";

const char* kFragmentShader = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorld;
uniform vec3 uColor;
uniform float uChecker;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(n, lightDir), 0.0);
    vec3 color = uColor;
    if (uChecker > 0.5) {
        // Nudge along the normal so a wall sitting exactly on a tile edge
        // doesn't flicker between two tiles.
        vec3 p = vWorld + n * 0.01;
        float c = mod(floor(p.x) + floor(p.y) + floor(p.z), 2.0);
        color *= 0.88 + 0.12 * c;
    }
    fragColor = vec4(color * (0.35 + 0.65 * diffuse), 1.0);
}
)";

// Unit cube from -1 to 1: 36 vertices of position + normal.
const float kCube[] = {
    // +X
    1,-1,-1, 1,0,0,  1, 1,-1, 1,0,0,  1, 1, 1, 1,0,0,  1,-1,-1, 1,0,0,  1, 1, 1, 1,0,0,  1,-1, 1, 1,0,0,
    // -X
   -1,-1,-1,-1,0,0, -1,-1, 1,-1,0,0, -1, 1, 1,-1,0,0, -1,-1,-1,-1,0,0, -1, 1, 1,-1,0,0, -1, 1,-1,-1,0,0,
    // +Y
   -1, 1,-1, 0,1,0, -1, 1, 1, 0,1,0,  1, 1, 1, 0,1,0, -1, 1,-1, 0,1,0,  1, 1, 1, 0,1,0,  1, 1,-1, 0,1,0,
    // -Y
   -1,-1,-1, 0,-1,0, 1,-1,-1, 0,-1,0, 1,-1, 1, 0,-1,0,-1,-1,-1, 0,-1,0, 1,-1, 1, 0,-1,0,-1,-1, 1, 0,-1,0,
    // +Z
   -1,-1, 1, 0,0,1,  1,-1, 1, 0,0,1,  1, 1, 1, 0,0,1, -1,-1, 1, 0,0,1,  1, 1, 1, 0,0,1, -1, 1, 1, 0,0,1,
    // -Z
   -1,-1,-1, 0,0,-1,-1, 1,-1, 0,0,-1, 1, 1,-1, 0,0,-1,-1,-1,-1, 0,0,-1, 1, 1,-1, 0,0,-1, 1,-1,-1, 0,0,-1,
};

GLuint compile(GLenum type, const char* src, std::string* error) {
    GLuint s = gl::CreateShader(type);
    gl::ShaderSource(s, 1, &src, nullptr);
    gl::CompileShader(s);
    GLint ok = 0;
    gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        gl::GetShaderInfoLog(s, sizeof(log), nullptr, log);
        if (error) *error = std::string("shader compile failed: ") + log;
        gl::DeleteShader(s);
        return 0;
    }
    return s;
}

} // namespace

bool Renderer::init(std::string* error) {
    const char* missing = nullptr;
    if (!gl::load(&missing)) {
        if (error) *error = std::string("missing OpenGL function: ") + missing;
        return false;
    }

    GLuint vs = compile(GL_VERTEX_SHADER, kVertexShader, error);
    if (!vs) return false;
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragmentShader, error);
    if (!fs) { gl::DeleteShader(vs); return false; }

    program_ = gl::CreateProgram();
    gl::AttachShader(program_, vs);
    gl::AttachShader(program_, fs);
    gl::LinkProgram(program_);
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);
    GLint linked = 0;
    gl::GetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = {};
        gl::GetProgramInfoLog(program_, sizeof(log), nullptr, log);
        if (error) *error = std::string("shader link failed: ") + log;
        return false;
    }

    uMvp_ = gl::GetUniformLocation(program_, "uMvp");
    uModel_ = gl::GetUniformLocation(program_, "uModel");
    uColor_ = gl::GetUniformLocation(program_, "uColor");
    uNormalSign_ = gl::GetUniformLocation(program_, "uNormalSign");
    uChecker_ = gl::GetUniformLocation(program_, "uChecker");

    gl::GenVertexArrays(1, &vao_);
    gl::BindVertexArray(vao_);
    gl::GenBuffers(1, &vbo_);
    gl::BindBuffer(GL_ARRAY_BUFFER, vbo_);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(kCube), kCube, GL_STATIC_DRAW);
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                            reinterpret_cast<const void*>(3 * sizeof(float)));
    gl::EnableVertexAttribArray(1);

    gl::Enable(GL_DEPTH_TEST);
    gl::Enable(GL_CULL_FACE);
    return true;
}

void Renderer::shutdown() {
    for (GpuModel& m : models_) {
        gl::DeleteBuffers(1, &m.vbo);
        gl::DeleteVertexArrays(1, &m.vao);
    }
    models_.clear();
    if (vbo_) gl::DeleteBuffers(1, &vbo_);
    if (vao_) gl::DeleteVertexArrays(1, &vao_);
    if (program_) gl::DeleteProgram(program_);
    vbo_ = vao_ = program_ = 0;
}

void Renderer::beginFrame(int width, int height, const Camera& camera) {
    gl::Viewport(0, 0, width, height);
    gl::ClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    viewProj_ = Mat4::perspective(camera.fovY, aspect, 0.02f, 100.0f) * camera.view();
    gl::UseProgram(program_);
    gl::BindVertexArray(vao_);
}

void Renderer::setCommonUniforms(const Mat4& model, Vec3 color, float normalSign, bool checker) {
    Mat4 mvp = viewProj_ * model;
    gl::UniformMatrix4fv(uMvp_, 1, GL_FALSE, mvp.m);
    gl::UniformMatrix4fv(uModel_, 1, GL_FALSE, model.m);
    gl::Uniform3f(uColor_, color.x, color.y, color.z);
    gl::Uniform1f(uNormalSign_, normalSign);
    gl::Uniform1f(uChecker_, checker ? 1.0f : 0.0f);
}

void Renderer::drawBox(Vec3 center, Vec3 halfExtents, Vec3 color, bool inside, bool checker) {
    Mat4 model = Mat4::translate(center) * Mat4::scale(halfExtents);
    gl::BindVertexArray(vao_);
    setCommonUniforms(model, color, inside ? -1.0f : 1.0f, checker);
    gl::CullFace(inside ? GL_FRONT : GL_BACK);
    gl::DrawArrays(GL_TRIANGLES, 0, 36);
}

int Renderer::uploadModel(const assets::MeshData& mesh) {
    if (mesh.vertices.empty()) return -1;
    GpuModel m;
    m.parts = mesh.parts;
    gl::GenVertexArrays(1, &m.vao);
    gl::BindVertexArray(m.vao);
    gl::GenBuffers(1, &m.vbo);
    gl::BindBuffer(GL_ARRAY_BUFFER, m.vbo);
    gl::BufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(float)),
                   mesh.vertices.data(), GL_STATIC_DRAW);
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                            reinterpret_cast<const void*>(3 * sizeof(float)));
    gl::EnableVertexAttribArray(1);
    gl::BindVertexArray(vao_);
    models_.push_back(m);
    return static_cast<int>(models_.size()) - 1;
}

void Renderer::drawModel(int id, const Mat4& model) {
    if (id < 0 || id >= static_cast<int>(models_.size())) return;
    const GpuModel& m = models_[id];
    gl::BindVertexArray(m.vao);
    gl::CullFace(GL_BACK);
    for (const auto& part : m.parts) {
        setCommonUniforms(model, part.color, 1.0f, false);
        gl::DrawArrays(GL_TRIANGLES, part.firstVertex, part.vertexCount);
    }
    gl::BindVertexArray(vao_);
}

bool Renderer::saveScreenshot(const char* path, int width, int height) {
    if (width <= 0 || height <= 0) return false;
    const int pitch = width * 3;
    std::vector<unsigned char> pixels(static_cast<size_t>(pitch) * height);
    gl::PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl::ReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL rows start at the bottom; BMP via SDL wants the top row first.
    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; ++y)
        std::copy_n(&pixels[static_cast<size_t>(height - 1 - y) * pitch], pitch,
                    &flipped[static_cast<size_t>(y) * pitch]);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(flipped.data(), width, height, 24, pitch,
                                                              SDL_PIXELFORMAT_RGB24);
    if (!surface) return false;
    bool ok = SDL_SaveBMP(surface, path) == 0;
    SDL_FreeSurface(surface);
    return ok;
}

} // namespace render
