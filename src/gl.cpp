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

void rgc::shader::src_from_mem(const char* buf) const
{
	glShaderSource(m_name, 1, &buf, nullptr);
}

void rgc::shader::src_from_file(const std::filesystem::path& filename) const
{
	std::ifstream f(filename);
	if (!f.is_open()) {
		throw std::invalid_argument("The given filename doesn't exist.");
	}
	std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	src_from_mem(src.c_str());
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

