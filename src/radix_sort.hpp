#pragma once

#include "gl.hpp"

#include <glad/glad.h>



namespace rgc
{
	struct radix_sorter
	{
		rgc::program m_count_program;
		rgc::program m_local_offsets_program;
		rgc::program m_reorder_program;

		size_t m_internal_arr_len;

		GLuint m_local_offsets_buf;
		GLuint m_glob_counts_buf;

		radix_sorter(size_t init_arr_len = 0);
		~radix_sorter();

		void resize_internal_buf(size_t arr_len);

		void sort(GLuint data_buf, GLuint idx_buf, size_t arr_len);
	};
}

