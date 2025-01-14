#pragma once

namespace retort::builtins {

const char *vertex_shader_filename = "<inline vertex shader>";
const char *fragment_shader_filename = "<inline fragment shader>";

const char *vertex_shader =
    "#version 450\n"
    "\n"
    "layout (location = 0) out vec3 fragColor;\n"
    "\n"
    "vec2 positions[4] = vec2[](vec2 (-1., -1.), vec2 (1., -1.), vec2 (1., "
    "1.), vec2 (-1., 1.));\n"
    "\n"
    "vec3 colors[3] = vec3[](vec3 (1.0, 0.0, 0.0), vec3 (0.0, 1.0, 0.0), vec3 "
    "(0.0, 0.0, 1.0));\n"
    "\n"
    "void main ()\n"
    "{\n"
    "\tgl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);\n"
    "\tfragColor = colors[gl_VertexIndex % 3];\n"
    "}";

const char *fragment_shader =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "\n"
    "layout (location = 0) in vec3 fragColor;\n"
    "\n"
    "layout (location = 0) out vec4 outColor;\n"
    "\n"
    "void main () { outColor = vec4 (fragColor, 1.0); }";

} // namespace retort::builtins
