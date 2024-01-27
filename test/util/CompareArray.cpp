#include "CompareArray.hpp"

#include "misc_utils.hpp"

using namespace glu;

CompareArray::CompareArray()
{
    Shader shader(GL_COMPUTE_SHADER);
    shader.source_from_file("resources/array_compare.comp");
    shader.compile();

    m_program.attach_shader(shader.handle());
    m_program.link();

    glGenBuffers(1, &m_errors_counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

CompareArray::~CompareArray()
{
    glDeleteBuffers(1, &m_errors_counter);
}

GLuint CompareArray::compare(GLuint array1, GLuint array2, size_t size)
{
    if (m_error_marks_buffer.size() < size)
        m_error_marks_buffer.resize(next_power_of_2(size));

    m_program.use();

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &k_zero);

    m_error_marks_buffer.clear(0);

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, array1, 0, (GLsizeiptr) (size * sizeof(GLuint)));
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, array2, 0, (GLsizeiptr) (size * sizeof(GLuint)));
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 2, m_errors_counter, 0, sizeof(GLuint));
    glBindBufferRange(
        GL_SHADER_STORAGE_BUFFER, 3, m_error_marks_buffer.handle(), 0, (GLsizeiptr) (size * sizeof(GLuint))
    );

    glUniform1ui(m_program.get_uniform_location("u_size"), size);

    glDispatchCompute(calc_workgroups_num(size), 1, 1);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
    GLuint errors_count;
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &errors_count);
    return errors_count;
}
