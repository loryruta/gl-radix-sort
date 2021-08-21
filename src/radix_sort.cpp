#include "radix_sort.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "renderdoc.hpp"

#define RGC_RADIX_SORT__BITSET_NUM        4
#define RGC_RADIX_SORT__BITSET_SIZE       GLuint(exp2(RGC_RADIX_SORT__BITSET_NUM))
#define RGC_RADIX_SORT__THREADS_PER_BLOCK 64
#define RGC_RADIX_SORT__ITEMS_PER_THREAD  4

rgc::radix_sorter::radix_sorter(size_t init_arr_len)
{
	{
		rgc::shader shader(GL_COMPUTE_SHADER);
		shader.src_from_txt_file("resources/radix_sort__count.comp");
		shader.compile();

		m_count_program.attach_shader(shader.m_name);
		m_count_program.link();
	}

	{
		rgc::shader shader(GL_COMPUTE_SHADER);
		shader.src_from_txt_file("resources/radix_sort__local_offsets.comp");
		shader.compile();

		m_local_offsets_program.attach_shader(shader.m_name);
		m_local_offsets_program.link();
	}

	{
		rgc::shader shader(GL_COMPUTE_SHADER);
		shader.src_from_txt_file("resources/radix_sort__reorder.comp");
		shader.compile();

		m_reorder_program.attach_shader(shader.m_name);
		m_reorder_program.link();
	}

	resize_internal_buf(init_arr_len);
}

rgc::radix_sorter::~radix_sorter()
{
}

GLuint calc_thread_blocks_num(size_t arr_len)
{
	return GLuint(ceil(float(arr_len) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
}

void rgc::radix_sorter::resize_internal_buf(size_t arr_len)
{
	m_internal_arr_len = arr_len;

	glGenBuffers(1, &m_local_offsets_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_local_offsets_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(calc_thread_blocks_num(arr_len) * RGC_RADIX_SORT__BITSET_SIZE * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &m_glob_counts_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glob_counts_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(RGC_RADIX_SORT__BITSET_SIZE * sizeof(GLuint)), nullptr, NULL);
}

void rgc::radix_sorter::sort(GLuint key_buf, GLuint val_buf, size_t arr_len)
{
	if (arr_len <= 1) {
		return;
	}

	if (m_internal_arr_len < arr_len) {
		resize_internal_buf(arr_len);
	}

	GLuint num_thread_blocks = calc_thread_blocks_num(arr_len);

	for (GLuint bitset_idx = 0; bitset_idx < (sizeof(GLuint) * 8) / RGC_RADIX_SORT__BITSET_NUM; bitset_idx++)
	{
		GLuint zero = 0;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glob_counts_buf);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_local_offsets_buf);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);

		// ------------------------------------------------------------------------------------------------
		// Per-block & global radix count
		// ------------------------------------------------------------------------------------------------

		m_count_program.use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_local_offsets_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_glob_counts_buf);

		glUniform1ui(m_count_program.get_uniform_location("u_arr_len"), arr_len);
		glUniform1ui(m_count_program.get_uniform_location("u_bitset_idx"), bitset_idx);

		rgc::renderdoc::watch(false, [&]()
		{
			glDispatchCompute(num_thread_blocks, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});

		// ------------------------------------------------------------------------------------------------
		// Local offsets (block-wide exclusive scan per radix)
		// ------------------------------------------------------------------------------------------------

		m_local_offsets_program.use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_local_offsets_buf);

		// Up-sweep (reduction)
		for (GLuint d = 0; d < GLuint(log2(num_thread_blocks)); d++)
		{
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_thread_blocks_num"), num_thread_blocks);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 0);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_depth"), d);

			rgc::renderdoc::watch(false, [&]()
			{
				auto workgroups_num = GLuint(ceil(float(num_thread_blocks) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
				glDispatchCompute(workgroups_num, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			});
		}

		// Clear last
		glUniform1ui(m_local_offsets_program.get_uniform_location("u_thread_blocks_num"), num_thread_blocks);
		glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 1);

		rgc::renderdoc::watch(false, [&]()
		{
			auto workgroups_num = GLuint(ceil(float(num_thread_blocks) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
			glDispatchCompute(workgroups_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});

		// Down-sweep
		for (GLint d = GLint(log2(num_thread_blocks)) - 1; d >= 0; d--)
		{
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_thread_blocks_num"), num_thread_blocks);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 2);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_depth"), d);

			rgc::renderdoc::watch(false, [&]()
			{
				auto workgroups_num = GLuint(ceil(float(num_thread_blocks) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
				glDispatchCompute(workgroups_num, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			});
		}

		// ------------------------------------------------------------------------------------------------
		// In thread-block reordering & writes to global memory
		// ------------------------------------------------------------------------------------------------

		m_reorder_program.use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_local_offsets_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_glob_counts_buf);

		glUniform1ui(m_reorder_program.get_uniform_location("u_arr_len"), arr_len);
		glUniform1ui(m_reorder_program.get_uniform_location("u_bitset_idx"), bitset_idx);

		rgc::renderdoc::watch(bitset_idx == 0 || bitset_idx == 1, [&]()
		{
			glDispatchCompute(num_thread_blocks, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});
	}

	rgc::program::unuse();

	glFinish();
}
