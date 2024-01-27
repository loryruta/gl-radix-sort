#include "CheckSorted.hpp"

using namespace glu;

CheckSorted::CheckSorted()
{
    Shader shader(GL_COMPUTE_SHADER);
    shader.src_from_txt_file("resources/sort_test_CheckSorted.comp.glsl");
    shader.compile();

    m_program.attach_shader(shader.handle());
    m_program.link();

    glGenBuffers(1, &m_errors_counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

CheckSorted::~CheckSorted()
{
    glDeleteBuffers(1, &m_errors_counter);
}

bool CheckSorted::is_sorted(GLuint arr, size_t arr_len, GLuint* errors_count)
{
    m_program.use();

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &k_zero);

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, arr, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, m_errors_counter);

    glUniform1ui(m_program.get_uniform_location("u_arr_len"), arr_len);

    glDispatchCompute(calc_workgroups_num(arr_len), 1, 1);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), errors_count);
    return *errors_count == 0;
}
