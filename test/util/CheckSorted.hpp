#pragma once

#include <cstdint>

#include "gl_utils.hpp"

namespace glu
{
    class CheckSorted
    {
    private:
        Program m_program;

        GLuint m_errors_counter;

    public:
        explicit CheckSorted();
        ~CheckSorted();

        bool is_sorted(GLuint arr, size_t arr_len, GLuint* errors_count);
    };
} // namespace glu