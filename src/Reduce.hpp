#ifndef GLU_REDUCE_HPP
#define GLU_REDUCE_HPP

#include "data_types.hpp"
#include "gl_utils.hpp"

namespace glu
{
    namespace detail
    {
        inline const char* k_reduction_shader_src = R"(
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_depth;

void main()
{
    uint step = 1 << (5 * u_depth);
    uint subgroup_i = gl_WorkGroupID.x * NUM_THREADS + gl_SubgroupID * gl_SubgroupSize;
    uint i = (subgroup_i + gl_SubgroupInvocationID) * step;
    if (i < u_count)
    {
        DATA_TYPE r = SUBGROUP_OPERATION(data[i]);
        if (gl_SubgroupInvocationID == 0)
        {
            data[i] = r;
        }
    }
}
)";
    }

    /// The operators that can be used for the reduction operation.
    enum ReduceOperator
    {
        ReduceOperator_Sum = 0,
        ReduceOperator_Mul,
        ReduceOperator_Min,
        ReduceOperator_Max
    };

    /// A class that implements the reduction operation.
    class Reduce
    {
    private:
        const DataType m_data_type;
        const ReduceOperator m_operator;
        const size_t m_num_threads;
        const size_t m_num_items;

        Program m_program;

    public:
        explicit Reduce(DataType data_type, ReduceOperator operator_) :
            m_data_type(data_type),
            m_operator(operator_),
            m_num_threads(1024),
            m_num_items(4)
        {
            std::string shader_src = "#version 460\n\n";

            shader_src += std::string("#define DATA_TYPE ") + to_glsl_type_str(m_data_type) + "\n";
            shader_src += std::string("#define NUM_THREADS ") + std::to_string(m_num_threads) + "\n";
            shader_src += std::string("#define NUM_ITEMS ") + std::to_string(m_num_items) + "\n";

            if (m_operator == ReduceOperator_Sum)
            {
                shader_src += "#define OPERATOR(a, b) (a + b)\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupAdd(value)\n";
            }
            else if (m_operator == ReduceOperator_Mul)
            {
                shader_src += "#define OPERATOR(a, b) (a * b)\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMul(value)\n";
            }
            else if (m_operator == ReduceOperator_Min)
            {
                shader_src += "#define OPERATOR(a, b) (min(a, b))\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMin(value)\n";
            }
            else if (m_operator == ReduceOperator_Max)
            {
                shader_src += "#define OPERATOR(a, b) (max(a, b))\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMax(value)\n";
            }
            else
            {
                GLU_FAIL("Invalid reduction operator: %d", m_operator);
            }

            shader_src += detail::k_reduction_shader_src;

            Shader shader(GL_COMPUTE_SHADER);
            shader.source_from_str(shader_src.c_str());
            shader.compile();

            m_program.attach_shader(shader);
            m_program.link();
        }

        ~Reduce() = default;

        void operator()(GLuint buffer, size_t count)
        {
            GLU_CHECK_ARGUMENT(buffer, "Invalid buffer");
            GLU_CHECK_ARGUMENT(count > 0, "Count must be greater than zero");

            m_program.use();

            glUniform1ui(m_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            for (int depth = 0;; depth++)
            {
                int step = 1 << (5 * depth);
                if (step >= count)
                    break;

                size_t level_count = count >> (5 * depth);

                glUniform1ui(m_program.get_uniform_location("u_depth"), depth);

                size_t num_workgroups = div_ceil(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }
    };
} // namespace glu

#endif // GLU_REDUCE_HPP
