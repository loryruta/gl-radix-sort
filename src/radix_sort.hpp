#pragma once

#include <glad/glad.h>

namespace r
{
	struct radix_sorter
	{
		GLuint m_scan_program;
		GLuint m_sum_program;
		GLuint m_write_program;

		size_t m_internal_max_buf_size;
		GLuint m_part_addr_buf;
		GLuint m_addr_buf;
		GLuint m_write_off_buf;

		radix_sorter(size_t init_internal_buf_size = 0);
		~radix_sorter();

		void sort(GLuint data_buf, GLuint idx_buf, size_t arr_size);
	};
}

