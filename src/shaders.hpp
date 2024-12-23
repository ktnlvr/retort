#pragma once

#include "utils.hpp"
#include <shaderc/shaderc.hpp>

namespace retort {

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

struct CompilationInfo {
  const char *filename;
  shaderc_shader_kind kind;
  std::vector<char> source;
  shaderc::CompileOptions options;
};

struct Compiler {
  shaderc::Compiler _compiler;

  std::vector<uint32_t> compile(const CompilationInfo &info) {
    shaderc::PreprocessedSourceCompilationResult result_pre =
        _compiler.PreprocessGlsl(info.source.data(), info.source.size(),
                                 info.kind, info.filename, info.options);
    if (result_pre.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      PANIC(result_pre.GetErrorMessage().c_str());
    }

    shaderc::AssemblyCompilationResult result_asm =
        _compiler.CompileGlslToSpvAssembly(
            result_pre.cbegin(), result_pre.cend() - result_pre.cbegin(),
            info.kind, info.filename, info.options);

    if (result_asm.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      PANIC(result_asm.GetErrorMessage().c_str());
    }

    shaderc::SpvCompilationResult result_spv = _compiler.AssembleToSpv(
        result_asm.cbegin(), result_asm.cend() - result_asm.cbegin());

    if (result_spv.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      PANIC(result_spv.GetErrorMessage().c_str());
    }

    return std::vector(result_spv.begin(), result_spv.end());
  }
};

} // namespace retort
