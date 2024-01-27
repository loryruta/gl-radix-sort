#pragma once

#include <cstdint>

#include "glad/glad.h"

#include "CompareArray.hpp"

namespace glu
{
class CheckPermuted
{
private:
  GLuint m_max_val;

  CompareArray m_array_compare;
  Program m_count_elements_program;

  GLuint m_original_counts_buf;
  GLuint m_permuted_counts_buf;

public:
  explicit CheckPermuted(GLuint max_val); // min_val = 0
  ~CheckPermuted() = default;

  void memorize_original_array(GLuint arr, size_t arr_len);
  void memorize_permuted_array(GLuint arr, size_t arr_len);
  bool is_permuted();

private:
  void count_elements(GLuint arr, size_t arr_len, GLuint counts_buf);
};

}
