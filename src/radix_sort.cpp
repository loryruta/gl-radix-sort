#include "radix_sort.hpp"

#include "renderdoc.hpp"

#define RGC_RADIX_SORT_BITSET_SIZE        GLuint(exp2(RGC_RADIX_SORT_BITSET_NUM))
#define RGC_RADIX_SORT__THREADS_PER_BLOCK 64
#define RGC_RADIX_SORT__ITEMS_PER_THREAD  4

const GLuint k_zero = 0;

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

template<typename T>
T round_to_power_of_2(T dim)
{
	return (T) exp2(ceil(log2(dim)));
}

void rgc::radix_sorter::resize_internal_buf(size_t arr_len)
{
	m_internal_arr_len = arr_len;

	glGenBuffers(1, &m_local_offsets_buf); // TODO TRY TO REMOVE THIS BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_local_offsets_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(round_to_power_of_2(calc_thread_blocks_num(arr_len)) * RGC_RADIX_SORT_BITSET_SIZE * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &m_glob_counts_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glob_counts_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(RGC_RADIX_SORT_BITSET_SIZE * sizeof(GLuint)), nullptr, NULL);

	glGenBuffers(1, &m_scratch_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_scratch_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (arr_len * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void rgc::radix_sorter::sort(GLuint key_buf, GLuint val_buf, size_t arr_len)
{
	if (arr_len <= 1) {
		return;
	}

	if (m_internal_arr_len < arr_len) {
		resize_internal_buf(arr_len);
	}

	// ------------------------------------------------------------------------------------------------
	// Sorting
	// ------------------------------------------------------------------------------------------------

	GLuint thread_blocks_num = calc_thread_blocks_num(arr_len);
	GLuint power_of_two_thread_blocks_num = round_to_power_of_2(thread_blocks_num);

	GLuint buffers[2] = { key_buf, m_scratch_buf };

	for (GLuint pass = 0; pass < RGC_RADIX_SORT_BITSET_COUNT; pass++)
	{
		// ------------------------------------------------------------------------------------------------
		// Initial clearing
		// ------------------------------------------------------------------------------------------------

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glob_counts_buf);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &k_zero);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_local_offsets_buf);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &k_zero);

		// ------------------------------------------------------------------------------------------------
		// Counting
		// ------------------------------------------------------------------------------------------------

		// Per-block & global radix count

		m_count_program.use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[pass % 2]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_local_offsets_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_glob_counts_buf);

		glUniform1ui(m_count_program.get_uniform_location("u_arr_len"), arr_len);
		glUniform1ui(m_count_program.get_uniform_location("u_bitset_idx"), pass);

		rgc::renderdoc::watch(true, [&]()
		{
			glDispatchCompute(thread_blocks_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});

		// ------------------------------------------------------------------------------------------------
		// Local offsets (block-wide exclusive scan per radix)
		// ------------------------------------------------------------------------------------------------

		m_local_offsets_program.use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_local_offsets_buf);

		// Up-sweep (reduction)
		for (GLuint d = 0; d < GLuint(log2(power_of_two_thread_blocks_num)); d++)
		{
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_arr_len"), power_of_two_thread_blocks_num);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 0);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_depth"), d);

			rgc::renderdoc::watch(false, [&]()
			{
				auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
				glDispatchCompute(workgroups_num, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			});
		}

		// Clear last
		glUniform1ui(m_local_offsets_program.get_uniform_location("u_arr_len"), power_of_two_thread_blocks_num);
		glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 1);

		rgc::renderdoc::watch(false, [&]()
		{
			auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
			glDispatchCompute(workgroups_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});

		// Down-sweep
		for (GLint d = GLint(log2(power_of_two_thread_blocks_num)) - 1; d >= 0; d--)
		{
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_arr_len"), power_of_two_thread_blocks_num);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 2);
			glUniform1ui(m_local_offsets_program.get_uniform_location("u_depth"), d);

			rgc::renderdoc::watch(false, [&]()
			{
				auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT__THREADS_PER_BLOCK * RGC_RADIX_SORT__ITEMS_PER_THREAD)));
				glDispatchCompute(workgroups_num, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			});
		}

		// ------------------------------------------------------------------------------------------------
		// Reordering
		// ------------------------------------------------------------------------------------------------

		// In thread-block reordering & writes to global memory

		m_reorder_program.use();

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffers[pass % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffers[(pass + 1) % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_local_offsets_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_glob_counts_buf);

		glUniform1ui(m_reorder_program.get_uniform_location("u_arr_len"), arr_len);
		glUniform1ui(m_reorder_program.get_uniform_location("u_bitset_idx"), pass);

		rgc::renderdoc::watch(true, [&]()
		{
			glDispatchCompute(thread_blocks_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});
	}

	rgc::program::unuse();
}
