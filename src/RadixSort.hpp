#ifndef GLU_RADIXSORT_HPP
#define GLU_RADIXSORT_HPP

#include "gl_utils.hpp"

namespace glu
{
    namespace detail
    {
        inline const char* k_radix_sort_counting_shader = R"(
layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer KeyBuffer
{
    uint b_key_buffer[];
};

layout(std430, binding = 1) buffer GlobalCountBuffer
{
    uint b_global_count_buffer[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_radix_shift;

shared uint s_local_count_buffer[16];

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (gl_LocalInvocationIndex < 16)
    {
        s_local_count_buffer[gl_LocalInvocationIndex] = 0;
    }

    barrier();

    if (i < u_count)
    {
        // Block-wide count on shared memory
        uint radix = (b_key_buffer[i] >> u_radix_shift) & 0xf;
        atomicAdd(s_local_count_buffer[radix], 1);
    }

    barrier();

    if (gl_LocalInvocationIndex < 16)
    {
        // Dispatch-wide count on global memory
        uint local_count = s_local_count_buffer[gl_LocalInvocationIndex];
        atomicAdd(b_global_count_buffer[gl_LocalInvocationIndex], local_count);
    }
}
)";

        inline const char* k_radix_sort_reordering_shader = R"(
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer SrcKeyBuffer
{
    uint b_src_key_buffer[];
};

layout(std430, binding = 1) readonly buffer SrcValBuffer
{
    uint b_src_val_buffer[];
};

layout(std430, binding = 2) writeonly buffer DstKeyBuffer
{
    uint b_dst_key_buffer[];
};

layout(std430, binding = 3) writeonly buffer DstValBuffer
{
    uint b_dst_val_buffer[];
};

layout(std430, binding = 4) readonly buffer GlobalCountBuffer
{
    uint b_global_count_buffer[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_radix_shift;

shared uint s_global_offset_buffer[16];
shared uint s_prefix_sum_buffer[NUM_THREADS];

void prefix_sum()  // Block-wide prefix sum (Blelloch scan)
{
    uint thread_i = gl_SubgroupID * gl_SubgroupSize + gl_SubgroupInvocationID;

    // Upsweep
    for (uint step = 1; step < NUM_THREADS; step <<= 1)
    {
        if (thread_i % 2 == 1)
        {
            uint i = thread_i * step + (step - 1);
            if (i < NUM_THREADS)
            {
                s_prefix_sum_buffer[i] = s_prefix_sum_buffer[i] + s_prefix_sum_buffer[i - step];
            }
        }

        barrier();
    }

    // Clear last
    if (thread_i == NUM_THREADS - 1) s_prefix_sum_buffer[thread_i] = 0;

    barrier();

    // Downsweep
    uint step = NUM_THREADS >> 1;
    for (; step > 0; step >>= 1)
    {
        uint i = thread_i * step + (step - 1);
        if (i + step < NUM_THREADS && thread_i % 2 == 0)
        {
            uint tmp = s_prefix_sum_buffer[i];
            s_prefix_sum_buffer[i] = s_prefix_sum_buffer[i + step];
            s_prefix_sum_buffer[i + step] = tmp + s_prefix_sum_buffer[i + step];
        }

        barrier();
    }
}

void main()
{
    uint thread_i = gl_SubgroupID * gl_SubgroupSize + gl_SubgroupInvocationID;
    uint i = gl_WorkGroupID.x * NUM_THREADS + thread_i;

    // Prefix sum on global counts to obtain global offsets
    if (gl_SubgroupID == 0 && gl_SubgroupInvocationID < 16)
    {
        uint v = subgroupExclusiveAdd(b_global_count_buffer[gl_SubgroupInvocationID]);
        s_global_offset_buffer[gl_SubgroupInvocationID] = v;
    }

    barrier();

    // Reordering
    for (uint radix = 0; radix < 16; radix++)
    {
        bool should_place = false;
        if (i < u_count)
        {
            should_place = ((b_src_key_buffer[i] >> u_radix_shift) & 0xf) == radix;
        }

        s_prefix_sum_buffer[thread_i] = should_place ? 1 : 0;

        barrier();

        // Prefix sum on local counts to obtain local offsets
        prefix_sum();

        if (should_place)
        {
            uint offset = s_global_offset_buffer[radix] + s_prefix_sum_buffer[thread_i];
            b_dst_key_buffer[offset] = b_src_key_buffer[i];
            b_dst_val_buffer[offset] = b_src_val_buffer[i];
        }
    }
}
)";
    } // namespace detail

    class RadixSort
    {
    private:
        Program m_count_program;
        Program m_reorder_program;

        const size_t m_num_threads;

        /// A buffer of size 16 * sizeof(GLuint) that stores the global counts of radixes.
        ShaderStorageBuffer m_count_buffer;

        ShaderStorageBuffer m_key_scratch_buffer;
        ShaderStorageBuffer m_val_scratch_buffer;

    public:
        explicit RadixSort() :
            m_num_threads(1024)
        {
            GLU_CHECK_ARGUMENT(is_power_of_2(m_num_threads), "Num threads must be a power of 2");

            m_count_buffer.resize(16 * sizeof(GLuint));

            std::string shader_src = "#version 460\n\n";
            shader_src += "#define NUM_THREADS " + std::to_string(m_num_threads) + "\n";

            { // Counting program
                Shader shader(GL_COMPUTE_SHADER);
                shader.source_from_str(shader_src + detail::k_radix_sort_counting_shader);
                shader.compile();

                m_count_program.attach_shader(shader.handle());
                m_count_program.link();
            }

            { // Reordering program
                Shader shader(GL_COMPUTE_SHADER);
                shader.source_from_str(shader_src + detail::k_radix_sort_reordering_shader);
                shader.compile();

                m_reorder_program.attach_shader(shader.handle());
                m_reorder_program.link();
            }
        }

        ~RadixSort() = default;

        void operator()(GLuint key_buffer, GLuint val_buffer, size_t count)
        {
            GLU_CHECK_ARGUMENT(key_buffer, "Invalid key buffer");
            GLU_CHECK_ARGUMENT(val_buffer, "Invalid value buffer");

            if (count <= 1)
                return; // Hey, that's already sorted x)

            size_t num_workgroups = div_ceil(count, size_t(1024));

            if (m_key_scratch_buffer.size() < count)
            {
                m_key_scratch_buffer.resize(next_power_of_2(count) * sizeof(GLuint), false);
            }

            if (m_val_scratch_buffer.size() < count)
            {
                m_val_scratch_buffer.resize(next_power_of_2(count) * sizeof(GLuint), false);
            }

            GLuint key_buffers[]{key_buffer, m_key_scratch_buffer.handle()};
            GLuint val_buffers[]{val_buffer, m_val_scratch_buffer.handle()};

            for (int i = 0; i < 8; i++)
            {
                // ---------------------------------------------------------------- Counting

                m_count_buffer.clear(0);

                m_count_program.use();

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_buffers[i % 2]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_count_buffer.handle());

                glUniform1ui(m_count_program.get_uniform_location("u_count"), count);
                glUniform1ui(m_count_program.get_uniform_location("u_radix_shift"), i << 2);

                glDispatchCompute(num_workgroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // ---------------------------------------------------------------- Reordering

                m_reorder_program.use();

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_buffers[i % 2]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, val_buffers[i % 2]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, key_buffers[(i + 1) % 2]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, val_buffers[(i + 1) % 2]);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_count_buffer.handle());

                glUniform1ui(m_reorder_program.get_uniform_location("u_count"), count);
                glUniform1ui(m_reorder_program.get_uniform_location("u_radix_shift"), i << 2);

                glDispatchCompute(num_workgroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }
    };
} // namespace glu

#endif // GLU_RADIXSORT_HPP
