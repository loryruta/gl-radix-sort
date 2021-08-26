#pragma once

#include "gl.hpp"

#include <array>

#include <glad/glad.h>

#define RGC_RADIX_SORT_BITSET_NUM   4
#define RGC_RADIX_SORT_BITSET_COUNT ((sizeof(GLuint) * 8) / RGC_RADIX_SORT_BITSET_NUM)

namespace rgc
{
	struct radix_sorter
	{
		rgc::program m_count_program;
		rgc::program m_local_offsets_program;
		rgc::program m_reorder_program;

		size_t m_internal_arr_len;

		GLuint m_local_offsets_buf;
		GLuint m_scratch_buf;
		GLuint m_glob_counts_buf;

		radix_sorter(size_t init_arr_len = 0);
		~radix_sorter();

		void resize_internal_buf(size_t arr_len);

		void sort(GLuint key_buf, GLuint val_buf, size_t arr_len);
	};
}

