#include "sort_test.hpp"

#include "renderdoc.hpp"

#define RGC_SORT_TEST_THREADS_NUM 64
#define RGC_SORT_TEST_ITEMS_NUM   4

const GLuint k_zero = 0;

GLuint calc_workgroups_num(GLuint req_dim)
{
	return (GLuint) (ceil(float(req_dim) / float(RGC_SORT_TEST_THREADS_NUM * RGC_SORT_TEST_ITEMS_NUM)));
}

// --------------------------------------------------------------------------------------------------------------------------------
// array_compare
// --------------------------------------------------------------------------------------------------------------------------------

rgc::radix_sort::array_compare::array_compare(size_t max_arr_len)
{
	{
		shader sh(GL_COMPUTE_SHADER);
		sh.src_from_txt_file("resources/sort_test_arr_compare.comp.glsl");
		sh.compile();

		m_program.attach_shader(sh.m_name);
		m_program.link();
	}

	glGenBuffers(1, &m_errors_counter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &m_error_marks_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_error_marks_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (max_arr_len * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

rgc::radix_sort::array_compare::~array_compare()
{
	glDeleteBuffers(1, &m_errors_counter);
	glDeleteBuffers(1, &m_error_marks_buf);
}

GLuint rgc::radix_sort::array_compare::compare(GLuint arr_1, GLuint arr_2, size_t arr_len)
{
	m_program.use();

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &k_zero);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_error_marks_buf);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &k_zero);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, arr_1, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, arr_2, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
	glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 2, m_errors_counter, 0, sizeof(GLuint));
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, m_error_marks_buf, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));

	glUniform1ui(m_program.get_uniform_location("u_arr_len"), arr_len);

	glDispatchCompute(calc_workgroups_num(arr_len), 1, 1);
	glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
	GLuint errors_count; glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &errors_count);
	return errors_count;
}

// --------------------------------------------------------------------------------------------------------------------------------
// check_permuted
// --------------------------------------------------------------------------------------------------------------------------------

rgc::radix_sort::check_permuted::check_permuted(GLuint max_val) :
	m_array_compare(max_val)
{
	m_max_val = max_val;

	{
		shader sh(GL_COMPUTE_SHADER);
		sh.src_from_txt_file("resources/sort_test_count_elements.comp.glsl");
		sh.compile();

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

rgc::radix_sort::check_permuted::~check_permuted()
{
	glDeleteBuffers(1, &m_original_counts_buf);
	glDeleteBuffers(1, &m_permuted_counts_buf);
}

void rgc::radix_sort::check_permuted::count_elements(GLuint arr, size_t arr_len, GLuint counts_buf)
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

void rgc::radix_sort::check_permuted::memorize_original_array(GLuint arr, size_t arr_len)
{
	count_elements(arr, arr_len, m_original_counts_buf);
}

void rgc::radix_sort::check_permuted::memorize_permuted_array(GLuint arr, size_t arr_len)
{
	count_elements(arr, arr_len, m_permuted_counts_buf);
}

bool rgc::radix_sort::check_permuted::is_permuted()
{
	return m_array_compare.compare(m_original_counts_buf, m_permuted_counts_buf, m_max_val) == 0;
}

// --------------------------------------------------------------------------------------------------------------------------------
// check_sorted
// --------------------------------------------------------------------------------------------------------------------------------

rgc::radix_sort::check_sorted::check_sorted()
{
	{
		shader sh(GL_COMPUTE_SHADER);
		sh.src_from_txt_file("resources/sort_test_check_sorted.comp.glsl");
		sh.compile();

		m_program.attach_shader(sh.m_name);
		m_program.link();
	}

	glGenBuffers(1, &m_errors_counter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

rgc::radix_sort::check_sorted::~check_sorted()
{
	glDeleteBuffers(1, &m_errors_counter);
}

bool rgc::radix_sort::check_sorted::is_sorted(GLuint arr, size_t arr_len, GLuint* errors_count)
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
