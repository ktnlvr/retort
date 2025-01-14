#pragma once

#include <cstdint>
#include <map>

namespace retort {

using TypeId = uint32_t;

enum struct TypeMarker : uint32_t {
  Void,
  Bool,
  Integer = 0x15,
  Float = 0x16,
  Vector = 0x17,
  Matrix,
  Array = 0x1C,
  Struct,
  Image,
  Function,
  Pointer = 0x20,
};

enum struct StorageClass : uint32_t {
  UniformConstant = 0,
  Input = 1,
  Uniform = 2,
  Output = 3,
  Workgroup = 4,
  CrossWorkgroup = 5,
  Private = 6,
  StorageBuffer = 12,
};

struct T {
  TypeMarker marker;
  union {
    struct {
      uint32_t width;
      bool sign;
    } integer;
    struct {
      uint32_t width;
      uint32_t encoding;
    } floating;
    struct {
      TypeId component_type;
      uint32_t component_count;
    } vector;
    struct {
      TypeId element_type;
      uint32_t length;
    } array;
    struct {
      TypeId column_type;
      uint32_t column_count;
    } matrix;
    struct {
      StorageClass storage_class;
      TypeId type;
    } pointer;
  };
};

struct TypeContext {
  std::map<TypeId, T> typemap;
  std::map<TypeId, std::string> names;
};

#define _opcode_reflection_case(_marker, code)                                 \
  case ((uint32_t)TypeMarker::##_marker): {                                    \
    t.marker = TypeMarker::##_marker;                                          \
    code;                                                                      \
    ctx.typemap[id] = t;                                                       \
    break;                                                                     \
  }

auto extract_type_info(const uint32_t *spirv, size_t count) -> TypeContext {
  TypeContext ctx;

  // TODO: check magic and stuff
  uint32_t offset = 5;

  while (offset < count) {
    uint32_t local_offset = 0;
    uint32_t instruction = spirv[offset];
    uint32_t length = instruction >> 16;
    uint32_t opcode = instruction & 0xFFFF;

    // eat the next word
    auto nom = [&]() { return spirv[offset + ++local_offset]; };

    T t = {};
    uint32_t id = nom();

    switch (opcode) {
      _opcode_reflection_case(Integer, {
        t.integer.width = nom();
        t.integer.sign = nom();
      });
      _opcode_reflection_case(Float, {
        t.floating.width = nom();

        if (length > 3)
          t.floating.encoding = nom();
        else
          t.floating.encoding = 0;
      });
      _opcode_reflection_case(Vector, {
        t.vector.component_type = nom();
        t.vector.component_count = nom();
      });
      _opcode_reflection_case(Array, {
        t.array.element_type = nom();
        t.array.length = nom();
      });
      _opcode_reflection_case(Pointer, {
        t.pointer.storage_class = (StorageClass)nom();
        t.pointer.type = nom();
      });
    }

    offset += length;
  }

  return ctx;
}

} // namespace retort
