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

const char *fragment_shader =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "\n"
    "layout (location = 0) in vec3 fragColor;\n"
    "\n"
    "layout (location = 0) out vec4 outColor;\n"
    "\n"
    "void main () { outColor = vec4 (fragColor, 1.0); }";

struct CompilationInfo {
  const char *filename;
  shaderc_shader_kind kind;
  std::vector<char> source;
  shaderc::CompileOptions options;

  CompilationInfo(const char *filename, shaderc_shader_kind kind,
                  const char *source)
      : filename(filename), kind(kind), source(source, source + strlen(source)),
        options() {
    options.SetTargetSpirv(shaderc_spirv_version_1_4);
    options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                 shaderc_env_version_vulkan_1_2);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetPreserveBindings(true);
  }
};

struct Compiler {
  shaderc::Compiler _compiler;

  std::vector<uint32_t> compile(const char *filename, shaderc_shader_kind kind,
                                const char *source) {
    CompilationInfo info(filename, kind, source);
    return compile(info);
  }

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

  std::vector<uint32_t> create_inline_vertex_shader_code() {
    return compile("inline_vertex", shaderc_vertex_shader, vertex_shader);
  }

  std::vector<uint32_t> create_inline_fragment_shader_code() {
    return compile("inline_fragment", shaderc_fragment_shader, fragment_shader);
  }
};

} // namespace retort
