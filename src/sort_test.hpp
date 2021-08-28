#pragma once

#include "generated/radix_sort.hpp"

namespace rgc::radix_sort
{
	class array_compare
	{
	private:
		program m_program;

		GLuint m_errors_counter;
		GLuint m_error_marks_buf;

	public:
		array_compare(size_t max_arr_len);
		~array_compare();

		GLuint compare(GLuint arr_1, GLuint arr_2, size_t arr_len);
	};

	class check_permuted
	{
	private:
		GLuint m_max_val;

		array_compare m_array_compare;
		program m_count_elements_program;

		GLuint m_original_counts_buf;
		GLuint m_permuted_counts_buf;

		void count_elements(GLuint arr, size_t arr_len, GLuint counts_buf);

	public:
		check_permuted(GLuint max_val); // min_val = 0
		~check_permuted();

		void memorize_original_array(GLuint arr, size_t arr_len);
		void memorize_permuted_array(GLuint arr, size_t arr_len);
		bool is_permuted();
	};

	class check_sorted
	{
	private:
		program m_program;

		GLuint m_errors_counter;

	public:
		check_sorted();
		~check_sorted();

		bool is_sorted(GLuint arr, size_t arr_len, GLuint* errors_count);
	};
}
