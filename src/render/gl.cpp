#include "render/gl.h"

#include <SDL.h>

namespace gl {
#define TOUR_GL_DEFINE(ret, name, params) name##Fn name = nullptr;
TOUR_GL_FUNCTIONS(TOUR_GL_DEFINE)
#undef TOUR_GL_DEFINE

bool load(const char** missing) {
#define TOUR_GL_LOAD(ret, name, params)                                              \
    name = reinterpret_cast<name##Fn>(SDL_GL_GetProcAddress("gl" #name));          \
    if (!name) {                                                                     \
        if (missing) *missing = "gl" #name;                                          \
        return false;                                                                \
    }
    TOUR_GL_FUNCTIONS(TOUR_GL_LOAD)
#undef TOUR_GL_LOAD
    return true;
}
} // namespace gl
