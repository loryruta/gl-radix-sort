#pragma once

#include <glad/glad.h> // Implement your own OpenGL functions loading library.

#include <array>
#include <fstream>
#include <filesystem>
#include <iostream>

#define RGC_RADIX_SORT_THREADS_PER_BLOCK 64
#define RGC_RADIX_SORT_ITEMS_PER_THREAD  4
#define RGC_RADIX_SORT_BITSET_NUM   4
#define RGC_RADIX_SORT_BITSET_COUNT ((sizeof(GLuint) * 8) / RGC_RADIX_SORT_BITSET_NUM)
#define RGC_RADIX_SORT_BITSET_SIZE  GLuint(exp2(RGC_RADIX_SORT_BITSET_NUM))

#ifdef RGC_RADIX_SORT_DEBUG
	#include "renderdoc.hpp"
	#define RGC_RADIX_SORT_RENDERDOC_WATCH(capture, f) rgc::renderdoc::watch(capture, f)
#else
	#define RGC_RADIX_SORT_RENDERDOC_WATCH(capture, f) f()
#endif

inline void __rgc_shader_injector_load_src(GLuint shader, char const* src)
{
	glShaderSource(shader, 1, &src, nullptr);
}

inline void __rgc_shader_injector_load_src_from_file(GLuint shader, std::filesystem::path const& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Failed to open file at: " << filename << std::endl;
		throw std::invalid_argument("Failed to open text file.");
	}

	std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	__rgc_shader_injector_load_src(shader, src.c_str());
}

#define RGC_SHADER_INJECTOR_INJECTION_POINT
#define RGC_SHADER_INJECTOR_LOAD_SRC(shader, path) __rgc_shader_injector_load_src_from_file(shader, path)

RGC_SHADER_INJECTOR_INJECTION_POINT

namespace rgc::radix_sort
{
	inline const GLuint k_zero = 0;

	struct shader
	{
		GLuint m_name;

		shader(GLenum type)
		{
			m_name = glCreateShader(type);
		}

		~shader()
		{
			glDeleteShader(m_name);
		}

		void src_from_txt(char const* txt) const
		{
			glShaderSource(m_name, 1, &txt, nullptr);
		}

		void src_from_txt_file(std::filesystem::path const& filename) const
		{
			std::ifstream file(filename);
			if (!file.is_open()) {
				throw std::invalid_argument("Failed to open text file.");
			}

			std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			src_from_txt(src.c_str());
		}

		std::string get_info_log() const
		{
			GLint max_len = 0;
			glGetShaderiv(m_name, GL_INFO_LOG_LENGTH, &max_len);

			std::vector<GLchar> log(max_len);
			glGetShaderInfoLog(m_name, max_len, nullptr, log.data());
			return std::string(log.begin(), log.end());
		}

		void compile() const
		{
			glCompileShader(m_name);

			GLint status;
			glGetShaderiv(m_name, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				std::cerr << "Failed to compile:\n" << get_info_log() << std::endl;
				throw std::runtime_error("Couldn't compile shader. See console logs for more information.");
			}
		}

	};

	struct program
	{
		GLuint m_name;

		program()
		{
			m_name = glCreateProgram();
		}

		~program()
		{
			glDeleteProgram(m_name);
		}

		void attach_shader(GLuint shader) const
		{
			glAttachShader(m_name, shader);
		}

		std::string get_info_log() const
		{
			GLint log_len = 0;
			glGetProgramiv(m_name, GL_INFO_LOG_LENGTH, &log_len);

			std::vector<GLchar> log(log_len);
			glGetProgramInfoLog(m_name, log_len, nullptr, log.data());
			return std::string(log.begin(), log.end());
		}

		void link() const
		{
			GLint status;
			glLinkProgram(m_name);
			glGetProgramiv(m_name, GL_LINK_STATUS, &status);
			if (status == GL_FALSE)
			{
				std::cerr << get_info_log() << std::endl;
				throw std::runtime_error("Couldn't link shader program.");
			}
		}

		GLint get_uniform_location(const char* name) const
		{
			GLint loc = glGetUniformLocation(m_name, name);
			if (loc < 0) {
				throw std::runtime_error("Couldn't find uniform. Is it unused maybe?");
			}
			return loc;
		}

		void use() const
		{
			glUseProgram(m_name);
		}

		static void unuse()
		{
			glUseProgram(NULL);
		}
	};

	struct sorter
	{
	private:
		program m_count_program;
		program m_local_offsets_program;
		program m_reorder_program;

		size_t m_internal_arr_len;

		GLuint m_local_offsets_buf;
		GLuint m_keys_scratch_buf;
		GLuint m_values_scratch_buf;
		GLuint m_glob_counts_buf;

		GLuint calc_thread_blocks_num(size_t arr_len)
		{
			return GLuint(ceil(float(arr_len) / float(RGC_RADIX_SORT_THREADS_PER_BLOCK * RGC_RADIX_SORT_ITEMS_PER_THREAD)));
		}

		template<typename T>
		T round_to_power_of_2(T dim)
		{
			return (T) exp2(ceil(log2(dim)));
		}

		void resize_internal_buf(size_t arr_len)
		{
			if (m_local_offsets_buf != NULL) glDeleteBuffers(1, &m_local_offsets_buf);
			if (m_glob_counts_buf != NULL)   glDeleteBuffers(1, &m_glob_counts_buf);
			if (m_keys_scratch_buf != NULL)  glDeleteBuffers(1, &m_keys_scratch_buf);
			if (m_values_scratch_buf != NULL)  glDeleteBuffers(1, &m_values_scratch_buf);

			m_internal_arr_len = arr_len;

			glGenBuffers(1, &m_local_offsets_buf); // TODO TRY TO REMOVE THIS BUFFER
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_local_offsets_buf);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(round_to_power_of_2(calc_thread_blocks_num(arr_len)) * RGC_RADIX_SORT_BITSET_SIZE * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

			glGenBuffers(1, &m_glob_counts_buf);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glob_counts_buf);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(RGC_RADIX_SORT_BITSET_SIZE * sizeof(GLuint)), nullptr, NULL);

			glGenBuffers(1, &m_keys_scratch_buf);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_keys_scratch_buf);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (arr_len * sizeof(GLuint)), nullptr, NULL);

			glGenBuffers(1, &m_values_scratch_buf);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_values_scratch_buf);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (arr_len * sizeof(GLuint)), nullptr, NULL);
		}

	public:
		sorter(size_t init_arr_len)
		{
			{
				shader sh(GL_COMPUTE_SHADER);
				RGC_SHADER_INJECTOR_LOAD_SRC(sh.m_name, "resources/radix_sort_count.comp.glsl");
				sh.compile();

				m_count_program.attach_shader(sh.m_name);
				m_count_program.link();
			}

			{
				shader sh(GL_COMPUTE_SHADER);
				RGC_SHADER_INJECTOR_LOAD_SRC(sh.m_name, "resources/radix_sort_local_offsets.comp.glsl");
				sh.compile();

				m_local_offsets_program.attach_shader(sh.m_name);
				m_local_offsets_program.link();
			}

			{
				shader sh(GL_COMPUTE_SHADER);
				RGC_SHADER_INJECTOR_LOAD_SRC(sh.m_name, "resources/radix_sort_reorder.comp.glsl");
				sh.compile();

				m_reorder_program.attach_shader(sh.m_name);
				m_reorder_program.link();
			}

			resize_internal_buf(init_arr_len);
		}

		~sorter()
		{
			if (m_local_offsets_buf != NULL) glDeleteBuffers(1, &m_local_offsets_buf);
			if (m_glob_counts_buf != NULL) glDeleteBuffers(1, &m_local_offsets_buf);
			if (m_keys_scratch_buf != NULL) glDeleteBuffers(1, &m_local_offsets_buf);
		}

		void sorter::sort(GLuint key_buf, GLuint val_buf, size_t arr_len)
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

			GLuint keys_buffers[2] = {key_buf, m_keys_scratch_buf};
			GLuint values_buffers[2] = {val_buf, m_values_scratch_buf};

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

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keys_buffers[pass % 2]);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_local_offsets_buf);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_glob_counts_buf);

				glUniform1ui(m_count_program.get_uniform_location("u_arr_len"), arr_len);
				glUniform1ui(m_count_program.get_uniform_location("u_bitset_idx"), pass);

				RGC_RADIX_SORT_RENDERDOC_WATCH(true, [&]()
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

					RGC_RADIX_SORT_RENDERDOC_WATCH(false, [&]()
					{
						auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT_THREADS_PER_BLOCK * RGC_RADIX_SORT_ITEMS_PER_THREAD)));
						glDispatchCompute(workgroups_num, 1, 1);
						glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					});
				}

				// Clear last
				glUniform1ui(m_local_offsets_program.get_uniform_location("u_arr_len"), power_of_two_thread_blocks_num);
				glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 1);

				RGC_RADIX_SORT_RENDERDOC_WATCH(false, [&]()
				{
					auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT_THREADS_PER_BLOCK * RGC_RADIX_SORT_ITEMS_PER_THREAD)));
					glDispatchCompute(workgroups_num, 1, 1);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				});

				// Down-sweep
				for (GLint d = GLint(log2(power_of_two_thread_blocks_num)) - 1; d >= 0; d--)
				{
					glUniform1ui(m_local_offsets_program.get_uniform_location("u_arr_len"), power_of_two_thread_blocks_num);
					glUniform1ui(m_local_offsets_program.get_uniform_location("u_op"), 2);
					glUniform1ui(m_local_offsets_program.get_uniform_location("u_depth"), d);

					RGC_RADIX_SORT_RENDERDOC_WATCH(false, [&]()
					{
						auto workgroups_num = GLuint(ceil(float(power_of_two_thread_blocks_num) / float(RGC_RADIX_SORT_THREADS_PER_BLOCK * RGC_RADIX_SORT_ITEMS_PER_THREAD)));
						glDispatchCompute(workgroups_num, 1, 1);
						glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					});
				}

				// ------------------------------------------------------------------------------------------------
				// Reordering
				// ------------------------------------------------------------------------------------------------

				// In thread-block reordering & writes to global memory

				m_reorder_program.use();

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, keys_buffers[pass % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, keys_buffers[(pass + 1) % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));

				if (val_buf != NULL)
				{
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, values_buffers[pass % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, values_buffers[(pass + 1) % 2], 0, (GLsizeiptr) (arr_len * sizeof(GLuint)));
				}

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_local_offsets_buf);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_glob_counts_buf);

				glUniform1ui(m_reorder_program.get_uniform_location("u_write_values"), val_buf != NULL);
				glUniform1ui(m_reorder_program.get_uniform_location("u_arr_len"), arr_len);
				glUniform1ui(m_reorder_program.get_uniform_location("u_bitset_idx"), pass);

				RGC_RADIX_SORT_RENDERDOC_WATCH(true, [&]()
				{
					glDispatchCompute(thread_blocks_num, 1, 1);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				});
			}

			program::unuse();
		}
	};
}

