#pragma once
// Tiny self-contained OpenGL 3.3 core loader. Every function (even GL 1.1 ones)
// is fetched through SDL_GL_GetProcAddress, so the same code works with
// opengl32 on Windows, libGL on Linux and mesa/EGL on the Switch.
#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define TOUR_GLAPI __stdcall
#else
#define TOUR_GLAPI
#endif

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef char GLchar;
typedef std::ptrdiff_t GLsizeiptr;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_RGB 0x1907
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82

// X-macro: return type, name, parameter list.
#define TOUR_GL_FUNCTIONS(X)                                                              \
    X(void, Clear, (GLbitfield mask))                                                     \
    X(void, ClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))                     \
    X(void, Viewport, (GLint x, GLint y, GLsizei w, GLsizei h))                           \
    X(void, Enable, (GLenum cap))                                                         \
    X(void, CullFace, (GLenum mode))                                                      \
    X(void, DrawArrays, (GLenum mode, GLint first, GLsizei count))                        \
    X(GLuint, CreateShader, (GLenum type))                                                \
    X(void, ShaderSource, (GLuint s, GLsizei n, const GLchar* const* src, const GLint* len)) \
    X(void, CompileShader, (GLuint s))                                                    \
    X(void, GetShaderiv, (GLuint s, GLenum p, GLint* out))                                \
    X(void, GetShaderInfoLog, (GLuint s, GLsizei max, GLsizei* len, GLchar* log))         \
    X(void, DeleteShader, (GLuint s))                                                     \
    X(GLuint, CreateProgram, (void))                                                      \
    X(void, AttachShader, (GLuint p, GLuint s))                                           \
    X(void, LinkProgram, (GLuint p))                                                      \
    X(void, GetProgramiv, (GLuint p, GLenum q, GLint* out))                               \
    X(void, GetProgramInfoLog, (GLuint p, GLsizei max, GLsizei* len, GLchar* log))        \
    X(void, UseProgram, (GLuint p))                                                       \
    X(void, DeleteProgram, (GLuint p))                                                    \
    X(GLint, GetUniformLocation, (GLuint p, const GLchar* name))                          \
    X(void, UniformMatrix4fv, (GLint loc, GLsizei n, GLboolean t, const GLfloat* v))      \
    X(void, Uniform3f, (GLint loc, GLfloat a, GLfloat b, GLfloat c))                      \
    X(void, Uniform1f, (GLint loc, GLfloat a))                                            \
    X(void, GenVertexArrays, (GLsizei n, GLuint* out))                                    \
    X(void, BindVertexArray, (GLuint vao))                                                \
    X(void, DeleteVertexArrays, (GLsizei n, const GLuint* v))                             \
    X(void, GenBuffers, (GLsizei n, GLuint* out))                                         \
    X(void, BindBuffer, (GLenum target, GLuint b))                                        \
    X(void, BufferData, (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
    X(void, DeleteBuffers, (GLsizei n, const GLuint* v))                                  \
    X(void, VertexAttribPointer, (GLuint i, GLint size, GLenum type, GLboolean norm,      \
                                  GLsizei stride, const void* ptr))                       \
    X(void, EnableVertexAttribArray, (GLuint i))                                         \
    X(void, PixelStorei, (GLenum pname, GLint param))                                     \
    X(void, ReadPixels, (GLint x, GLint y, GLsizei w, GLsizei h, GLenum format,          \
                         GLenum type, void* data))

namespace gl {
#define TOUR_GL_DECLARE(ret, name, params) \
    typedef ret(TOUR_GLAPI* name##Fn) params; \
    extern name##Fn name;
TOUR_GL_FUNCTIONS(TOUR_GL_DECLARE)
#undef TOUR_GL_DECLARE

// Call after the GL context exists. Returns false and names the first
// missing function if anything can't be loaded.
bool load(const char** missing);
} // namespace gl
