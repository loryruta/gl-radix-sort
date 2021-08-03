#include "radix_sort.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "renderdoc.hpp"

#define RADIX_SORT_WORKGROUP_SIZE 256
#define RADIX_SORT_BITSET_NUM     4

GLuint compile_shader_from_file_and_validate(GLenum shader_type, std::filesystem::path const& filename)
{
	GLuint shader = glCreateShader(shader_type);

	std::ifstream f(filename);
	if (!f.is_open()) {
		throw std::invalid_argument("The given filename doesn't exist.");
	}
	std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	char const* src_ptr = src.c_str();
	glShaderSource(shader, 1, &src_ptr, nullptr);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint max_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

		std::vector<GLchar> log(max_len);
		glGetShaderInfoLog(shader, max_len, nullptr, log.data());

		std::cerr << filename << " failed to compile: " << std::endl << std::string(log.begin(), log.end()) << std::endl;

		throw std::runtime_error("Couldn't compile shader. See console logs for more information.");
	}

	return shader;
}

std::string get_program_info_log(GLuint program)
{
	GLint log_len = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

	std::vector<GLchar> log(log_len);
	glGetProgramInfoLog(program, log_len, nullptr, log.data());
	return std::string(log.begin(), log.end());
}

void link_program_and_validate(GLuint program)
{
	GLint status;
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		std::cerr << get_program_info_log(program) << std::endl;
		throw std::runtime_error("Couldn't link shader program.");
	}
}

GLint get_uniform_location(GLuint program, char const* name)
{
	GLint loc = glGetUniformLocation(program, name);
	if (loc < 0)
	{
		printf("Couldn't find uniform: `%s`\n", name);
		fflush(stdout);

		throw std::runtime_error("Couldn't find uniform. Is it unused maybe?");
	}
	return loc;
}

r::radix_sorter::radix_sorter(size_t init_internal_buf_size)
{
	GLuint shader;

	// Scan
	m_scan_program = glCreateProgram();
	shader = compile_shader_from_file_and_validate(GL_COMPUTE_SHADER, "resources/radix_sort__scan.comp");
	glAttachShader(m_scan_program, shader);

	link_program_and_validate(m_scan_program);

	// Sum
	m_sum_program = glCreateProgram();
	shader = compile_shader_from_file_and_validate(GL_COMPUTE_SHADER, "resources/radix_sort__sum.comp");
	glAttachShader(m_sum_program, shader);

	link_program_and_validate(m_sum_program);

	// Write
	m_write_program = glCreateProgram();
	shader = compile_shader_from_file_and_validate(GL_COMPUTE_SHADER, "resources/radix_sort__write.comp");
	glAttachShader(m_write_program, shader);

	link_program_and_validate(m_write_program);

	// todo clear shaders

	m_internal_max_buf_size = init_internal_buf_size;

	glGenBuffers(1, &m_addr_buf); // Addresses buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_addr_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (m_internal_max_buf_size * size_t(exp2(RADIX_SORT_BITSET_NUM)) * sizeof(GLuint)), nullptr, NULL);

	glGenBuffers(1, &m_part_addr_buf); // Partition addresses buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_part_addr_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (m_internal_max_buf_size * size_t(exp2(RADIX_SORT_BITSET_NUM)) * sizeof(GLuint)), nullptr, NULL);

	glGenBuffers(1, &m_write_off_buf); // Write offset
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_write_off_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) (2 * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

r::radix_sorter::~radix_sorter()
{
	glDeleteProgram(m_scan_program);
	glDeleteProgram(m_sum_program);
	glDeleteProgram(m_write_program);

	glDeleteBuffers(1, &m_part_addr_buf);
	glDeleteBuffers(1, &m_addr_buf);
}


void r::radix_sorter::sort(GLuint key_data_buf, GLuint val_data_buf, size_t arr_size)
{
	if (m_internal_max_buf_size < arr_size) {
		throw std::runtime_error("Internal buffer sizes are too small. Consider using much more memory.");
	}

	auto part_num = GLuint(ceil(float(arr_size) / float(RADIX_SORT_WORKGROUP_SIZE)));
	auto bitset_num = GLuint(ceil(float(sizeof(GLuint) * 8) / float(RADIX_SORT_BITSET_NUM)));

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_write_off_buf);
	GLuint zero = 0; glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

	for (int i = 0; i < bitset_num; i++)
	{
		// Scan
		glUseProgram(m_scan_program);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_data_buf);
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, val_data_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_part_addr_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_addr_buf);

		glUniform1ui(get_uniform_location(m_scan_program, "u_arr_size"), arr_size);
		glUniform1ui(get_uniform_location(m_scan_program, "u_bitset_idx"), i);

		renderdoc::watch(i == 0, [&]()
		{
			glDispatchCompute(part_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});

		// Sum
		glUseProgram(m_sum_program);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_part_addr_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_addr_buf);

		for (int part_idx = 0; part_idx < (part_num - 1); part_idx++)
		{
			glUniform1ui(get_uniform_location(m_sum_program, "u_to_sum_part_idx"), part_idx);

			renderdoc::watch(i == 0 && part_idx == (part_num - 2), [&]()
			{
				glDispatchCompute(part_num, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			});
		}

		// Write
		glUseProgram(m_write_program);

		// Copy key_buf on key_cpy_buf
		glBindBuffer(GL_COPY_READ_BUFFER, key_data_buf);
		glBindBuffer(GL_COPY_WRITE_BUFFER, m_part_addr_buf); // part_addr_buf = key_cpy_buf
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, GLsizeiptr(arr_size * sizeof(GLuint)));

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, key_data_buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, val_data_buf);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, m_part_addr_buf, 0, GLsizeiptr(arr_size * sizeof(GLuint))); // key_cpy_buf
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_addr_buf);

		glUniform1ui(get_uniform_location(m_write_program, "u_arr_size"), arr_size);
		glUniform1ui(get_uniform_location(m_write_program, "u_bitset_idx"), i);

		renderdoc::watch(i == (bitset_num - 1), [&]()
		{
			glDispatchCompute(part_num, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		});
	}

	glFinish(); // todo use fences
}
