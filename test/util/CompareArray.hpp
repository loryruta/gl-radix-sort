#pragma once

#include <cstdint>

#include "glad/glad.h"

#include "gl_utils.hpp"

namespace glu
{
    /// A class used to check whether two arrays are equal.
    class CompareArray
    {
    private:
        Program m_program;

        GLuint m_errors_counter;
        ShaderStorageBuffer m_error_marks_buffer; ///< A buffer used to tell the mismatching locations.

    public:
        explicit CompareArray();
        ~CompareArray();

        /// Checks whether the two arrays (must be SSBOs) are equal.
        /// @returns the number of mismatching values
        GLuint compare(GLuint array1, GLuint array2, size_t size);
    };
} // namespace glu