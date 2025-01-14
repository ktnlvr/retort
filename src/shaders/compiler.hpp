#pragma once

#include <shaderc/shaderc.hpp>

#include "../error.hpp"
#include "../utils.hpp"

#include "./builtins.hpp"

namespace retort {

struct CompilationError {
  CompilationError(const char *messages) : messages(messages) {}

  std::string messages;
};

using CompilationResult = Result<std::vector<uint32_t>, CompilationError>;

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

  auto compile(const char *filename, shaderc_shader_kind kind,
               const char *source) -> CompilationResult {
    CompilationInfo info(filename, kind, source);
    return compile(info);
  }

  auto compile(const CompilationInfo &info) -> CompilationResult {
    shaderc::PreprocessedSourceCompilationResult result_pre =
        _compiler.PreprocessGlsl(info.source.data(), info.source.size(),
                                 info.kind, info.filename, info.options);
    if (result_pre.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      return CompilationError(result_pre.GetErrorMessage().c_str());
    }

    shaderc::AssemblyCompilationResult result_asm =
        _compiler.CompileGlslToSpvAssembly(
            result_pre.cbegin(), result_pre.cend() - result_pre.cbegin(),
            info.kind, info.filename, info.options);

    if (result_asm.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      return CompilationError(result_asm.GetErrorMessage().c_str());
    }

    shaderc::SpvCompilationResult result_spv = _compiler.AssembleToSpv(
        result_asm.cbegin(), result_asm.cend() - result_asm.cbegin());

    if (result_spv.GetCompilationStatus() !=
        shaderc_compilation_status_success) {
      return CompilationError(result_spv.GetErrorMessage().c_str());
    }

    return std::vector(result_spv.begin(), result_spv.end());
  }

  auto compile_fragment_shader(const char *filename, const char *source)
      -> CompilationResult {
    return compile(filename, shaderc_fragment_shader, source);
  }

  auto create_inline_vertex_shader_code() -> CompilationResult {
    return compile(builtins::vertex_shader_filename, shaderc_vertex_shader,
                   builtins::vertex_shader);
  }

  auto create_inline_fragment_shader_code() -> CompilationResult {
    return compile_fragment_shader(builtins::fragment_shader_filename,
                                   builtins::fragment_shader);
  }
};

} // namespace retort
