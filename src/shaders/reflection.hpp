#pragma once

#include "../utils.hpp"

#include <cstdint>
#include <map>

#include "spirv_reflect.h"
#include "../../build/_deps/fetch_spirv_reflect-src/spirv_reflect.h"

#define EXPECT_SPV_REFLECT_OK(result)                                          \
  EXPECT(result == SPV_REFLECT_RESULT_SUCCESS)

namespace retort {

auto extract_type_info(const uint32_t *spirv, size_t count) -> int32_t {
  SpvReflectShaderModule module;
  SpvReflectResult result =
      spvReflectCreateShaderModule(count * sizeof(uint32_t), spirv, &module);
  EXPECT_SPV_REFLECT_OK(result);

  uint32_t var_count = 0;
  result = spvReflectEnumerateInputVariables(&module, &var_count, nullptr);
  EXPECT_SPV_REFLECT_OK(result);

  SpvReflectInterfaceVariable **input_vars =
      (SpvReflectInterfaceVariable **)malloc(
          var_count * sizeof(SpvReflectInterfaceVariable *));
  result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
  EXPECT_SPV_REFLECT_OK(result);

  for (uint32_t i = 0; i < var_count; i++) {
    std::cout << input_vars[i]->name << std::endl;
  }

  free(input_vars);
  spvReflectDestroyShaderModule(&module);

  return 0;
}

} // namespace retort
