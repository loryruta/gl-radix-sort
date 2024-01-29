#ifndef GLU_BLELLOCHSCAN_HPP
#define GLU_BLELLOCHSCAN_HPP

#include <string>

#include "Reduce.hpp"
#include "data_types.hpp"

namespace glu
{
    namespace detail
    {
        inline const char* k_upsweep_shader_src = R"(
#extension GL_KHR_shader_subgroup_shuffle_relative : require

layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_step;

void main()
{
    uint partition_i = gl_WorkGroupID.y;
    uint thread_i = gl_WorkGroupID.x * NUM_THREADS + gl_SubgroupID * gl_SubgroupSize + gl_SubgroupInvocationID;
    uint i = partition_i * u_count + thread_i * u_step + u_step - 1;
    uint end_i = (partition_i + 1) * u_count;
    if (i < end_i)
    {
        DATA_TYPE lval = subgroupShuffleUp(data[i], 1);
        DATA_TYPE r = OPERATION(data[i], lval);
        if (i == end_i - 1)  // Clear last
        {
            data[i] = IDENTITY;
        }
        else if (gl_SubgroupInvocationID % 2 == 1)
        {
            data[i] = r;
        }
    }
}
)";

        inline const char* k_downsweep_shader_src = R"(
layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_step;

void main()
{
    uint partition_i = gl_WorkGroupID.y;
    uint i = partition_i * u_count + gl_GlobalInvocationID.x * (u_step << 1) + (u_step - 1);
    uint next_i = i + u_step;
    uint end_i = (partition_i + 1) * u_count;
    if (next_i < end_i)
    {
        DATA_TYPE tmp = data[i];
        data[i] = data[next_i];
        data[next_i] = data[next_i] + tmp;
    }
    else if (i < end_i)
    {
        data[i] = IDENTITY;
    }
}
)";
    } // namespace detail

    /// A class that implements Blelloch scan algorithm (exclusive prefix sum).
    class BlellochScan
    {
    private:
        const DataType m_data_type;
        const size_t m_num_threads;
        const size_t m_num_items;

        Program m_upsweep_program;
        Program m_downsweep_program;

    public:
        explicit BlellochScan(DataType data_type) :
            m_data_type(data_type),
            m_num_threads(1024),
            m_num_items(4)
        {
            std::string shader_src = "#version 460\n\n";

            shader_src += std::string("#define DATA_TYPE ") + to_glsl_type_str(m_data_type) + "\n";
            shader_src += "#define OPERATION(a, b) (a + b)\n";
            shader_src += "#define IDENTITY 0\n";
            shader_src += std::string("#define NUM_THREADS ") + std::to_string(m_num_threads) + "\n";
            shader_src += std::string("#define NUM_ITEMS ") + std::to_string(m_num_items) + "\n";

            { // Upsweep program
                Shader upsweep_shader(GL_COMPUTE_SHADER);
                upsweep_shader.source_from_str((shader_src + detail::k_upsweep_shader_src).c_str());
                upsweep_shader.compile();

                m_upsweep_program.attach_shader(upsweep_shader);
                m_upsweep_program.link();
            }

            { // Downsweep program
                Shader downsweep_program(GL_COMPUTE_SHADER);
                downsweep_program.source_from_str((shader_src + detail::k_downsweep_shader_src).c_str());
                downsweep_program.compile();

                m_downsweep_program.attach_shader(downsweep_program);
                m_downsweep_program.link();
            }
        }

        ~BlellochScan() = default;

        /// Runs Blelloch exclusive scan on multiple partitions.
        ///
        /// @param buffer the input GLuint buffer
        /// @param count the number of GLuint in the buffer (must be a power of 2)
        /// @param num_partitions the number of partitions (must be adjacent)
        void operator()(GLuint buffer, size_t count, size_t num_partitions = 1)
        {
            GLU_CHECK_ARGUMENT(buffer, "Invalid buffer");
            GLU_CHECK_ARGUMENT(count > 0, "Count must be greater than zero");
            GLU_CHECK_ARGUMENT(is_power_of_2(count), "Count must be a power of 2"); // TODO Remove this requirement
            GLU_CHECK_ARGUMENT(num_partitions >= 1, "Num of partitions must be >= 1");

            upsweep(buffer, count, num_partitions); // Also clear last
            downsweep(buffer, count, num_partitions);
        }

    private:
        void upsweep(GLuint buffer, size_t count, size_t num_partitions) // Also clear last
        {
            m_upsweep_program.use();

            glUniform1ui(m_upsweep_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            int step = 1;
            int level_count = (int) count;
            while (true)
            {
                glUniform1ui(m_upsweep_program.get_uniform_location("u_step"), step);

                size_t num_workgroups = div_ceil<size_t>(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, num_partitions, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                step <<= 1;

                level_count >>= 1;

                if (level_count <= 1)
                    break;
            }
        }

        void downsweep(GLuint buffer, size_t count, size_t num_partitions)
        {
            m_downsweep_program.use();

            glUniform1ui(m_downsweep_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            int step = next_power_of_2(int(count)) >> 1;
            size_t level_count = 1;
            while (true)
            {
                glUniform1ui(m_downsweep_program.get_uniform_location("u_step"), step);

                size_t num_workgroups = div_ceil(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, num_partitions, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                step >>= 1;
                level_count <<= 1;
                if (step == 0)
                    break;
            }
        }
    };
} // namespace glu

#endif // GLU_BLELLOCHSCAN_HPP
