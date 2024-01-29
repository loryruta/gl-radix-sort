#ifndef GLU_DATA_TYPES_HPP
#define GLU_DATA_TYPES_HPP

#include "errors.hpp"

namespace glu
{
    enum DataType
    {
        DataType_Float = 0,
        DataType_Double,
        DataType_Int,
        DataType_Uint,
        DataType_Vec2,
        DataType_Vec4,
        DataType_DVec2,
        DataType_DVec4,
        DataType_UVec2,
        DataType_UVec4,
        DataType_IVec2,
        DataType_IVec4
    };

    inline const char* to_glsl_type_str(DataType data_type)
    {
        // clang-format off
        if (data_type == DataType_Float)       return "float";
        else if (data_type == DataType_Double) return "double";
        else if (data_type == DataType_Int)    return "int";
        else if (data_type == DataType_Uint)   return "uint";
        else if (data_type == DataType_Vec2)   return "vec2";
        else if (data_type == DataType_Vec4)   return "vec4";
        else if (data_type == DataType_DVec2)  return "dvec2";
        else if (data_type == DataType_DVec4)  return "dvec4";
        else if (data_type == DataType_UVec2)  return "uvec2";
        else if (data_type == DataType_UVec4)  return "uvec4";
        else if (data_type == DataType_IVec2)  return "ivec2";
        else if (data_type == DataType_IVec4)  return "ivec4";
        else
        {
            GLU_FAIL("Invalid data type: %d", data_type);
        }
        // clang-format on
    }

} // namespace glu

#endif // GLU_DATA_TYPES_HPP
