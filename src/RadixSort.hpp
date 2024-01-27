#ifndef GLU_RADIXSORT_HPP
#define GLU_RADIXSORT_HPP

#include "gl_utils.hpp"

namespace glu
{
    class RadixSort
    {
    private:
        Program m_count_program;
        Program m_local_offsets_program;
        Program m_reorder_program;

        size_t m_internal_arr_len;

        GLuint m_local_offsets_buf;
        GLuint m_keys_scratch_buf;
        GLuint m_values_scratch_buf;
        GLuint m_glob_counts_buf;

    public:
        RadixSort(size_t init_arr_len)
        {
            {
                Shader shader(GL_COMPUTE_SHADER);
                RGC_SHADER_INJECTOR_LOAD_SRC(shader.handle(), "resources/radix_sort_count.comp.glsl");
                shader.compile();

                m_count_program.attach_shader(sh.m_name);
                m_count_program.link();
            }

            {
                Shader shader(GL_COMPUTE_SHADER);
                RGC_SHADER_INJECTOR_LOAD_SRC(shader.m_name, "resources/radix_sort_local_offsets.comp.glsl");
                shader.compile();

                m_local_offsets_program.attach_shader(shader.m_name);
                m_local_offsets_program.link();
            }

            {
                Shader shader(GL_COMPUTE_SHADER);
                RGC_SHADER_INJECTOR_LOAD_SRC(shader.m_name, "resources/radix_sort_reorder.comp.glsl");
                shader.compile();

                m_reorder_program.attach_shader(shader.m_name);
                m_reorder_program.link();
            }
        }
    };

}

#endif // GLU_RADIXSORT_HPP
