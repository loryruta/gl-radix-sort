#include "CheckPermuted.hpp"

#include "misc_utils.hpp"

using namespace glu;

CheckPermuted::CheckPermuted(GLuint max_val) : m_array_compare(max_val)
{
  m_max_val = max_val;

  {
    Shader shader(GL_COMPUTE_SHADER);
    shader.src_from_txt_file("resources/sort_test_count_elements.comp.glsl");
    shader.compile();

    m_count_elements_program.attach_shader(sh.m_name);
    m_count_elements_program.link();
  }

  glGenBuffers(1, &m_original_counts_buf);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_original_counts_buf);
  glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizei) (max_val * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

  glGenBuffers(1, &m_permuted_counts_buf);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_permuted_counts_buf);
  glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizei) (max_val * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

CheckPermuted::~CheckPermuted()
{
  glDeleteBuffers(1, &m_original_counts_buf);
  glDeleteBuffers(1, &m_permuted_counts_buf);
}

void CheckPermuted::count_elements(GLuint arr, size_t arr_len, GLuint counts_buf)
{
  m_count_elements_program.use();

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, counts_buf);
  glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &k_zero);

  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, arr, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, counts_buf);

  glUniform1ui(m_count_elements_program.get_uniform_location("u_arr_len"), arr_len);
  glUniform1ui(m_count_elements_program.get_uniform_location("u_max_val"), m_max_val);

  rgc::renderdoc::watch(false, [&]() {
    glDispatchCompute(calc_workgroups_num(arr_len), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  });
}

void CheckPermuted::memorize_original_array(GLuint arr, size_t arr_len)
{
  count_elements(arr, arr_len, m_original_counts_buf);
}

void CheckPermuted::memorize_permuted_array(GLuint arr, size_t arr_len)
{
  count_elements(arr, arr_len, m_permuted_counts_buf);
}

bool CheckPermuted::is_permuted()
{
  return m_array_compare.compare(m_original_counts_buf, m_permuted_counts_buf, m_max_val) == 0;
}


