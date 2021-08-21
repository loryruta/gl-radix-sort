#include "gl.hpp"

#include <fstream>
#include <iostream>

// ------------------------------------------------------------------------------------------------
// shader
// ------------------------------------------------------------------------------------------------

rgc::shader::shader(GLenum type)
{
	m_name = glCreateShader(type);
}

rgc::shader::~shader()
{
	glDeleteShader(m_name);
}

void rgc::shader::src_from_txt(char const* txt) const
{
	glShaderSource(m_name, 1, &txt, nullptr);
}

void rgc::shader::src_from_spirv(void const* buf, GLsizei buf_len) const
{
	glShaderBinary(1, &m_name, GL_SHADER_BINARY_FORMAT_SPIR_V, buf, buf_len);
	glSpecializeShader(m_name, "main", 0, nullptr, nullptr);
}

void rgc::shader::src_from_txt_file(std::filesystem::path const& filename) const
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::invalid_argument("Failed to open text file.");
	}

	std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	src_from_txt(src.c_str());
}

void rgc::shader::src_from_spirv_file(std::filesystem::path const& filename) const
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open binary file.");
	}

	auto file_size = file.tellg();
	std::vector<char> buf(file_size);

	file.seekg(0);
	file.read(buf.data(), file_size);

	src_from_spirv(buf.data(), (GLsizei) buf.size());
}

void rgc::shader::compile() const
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

std::string rgc::shader::get_info_log() const
{
	GLint max_len = 0;
	glGetShaderiv(m_name, GL_INFO_LOG_LENGTH, &max_len);

	std::vector<GLchar> log(max_len);
	glGetShaderInfoLog(m_name, max_len, nullptr, log.data());
	return std::string(log.begin(), log.end());
}

// ------------------------------------------------------------------------------------------------
// program
// ------------------------------------------------------------------------------------------------

rgc::program::program()
{
	m_name = glCreateProgram();
}

rgc::program::~program()
{
	glDeleteProgram(m_name);
}

void rgc::program::attach_shader(GLuint shader) const
{
	glAttachShader(m_name, shader);
}

void rgc::program::link() const
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

std::string rgc::program::get_info_log() const
{
	GLint log_len = 0;
	glGetProgramiv(m_name, GL_INFO_LOG_LENGTH, &log_len);

	std::vector<GLchar> log(log_len);
	glGetProgramInfoLog(m_name, log_len, nullptr, log.data());
	return std::string(log.begin(), log.end());
}

void rgc::program::use() const
{
	glUseProgram(m_name);
}

void rgc::program::unuse()
{
	glUseProgram(NULL);
}

GLint rgc::program::get_uniform_location(const char* name) const
{
	GLint loc = glGetUniformLocation(m_name, name);
	if (loc < 0) {
		//throw std::runtime_error("Couldn't find uniform. Is it unused maybe?");
	}
	return loc;
}

