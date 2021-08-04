#pragma once

#include <filesystem>

#include <glad/glad.h>

namespace rgc
{
	struct shader
	{
		GLuint m_name;

		shader(GLenum type);
		~shader();

		void src_from_mem(char const* buf) const;
		void src_from_file(std::filesystem::path const& filename) const;

		void compile() const;

		std::string get_info_log() const;
	};

	struct program
	{
		GLuint m_name;

		program();
		~program();

		void attach_shader(GLuint shader) const;

		void link() const;

		std::string get_info_log() const;

		void use() const;
		static void unuse();

		GLint get_uniform_location(char const* name) const;
	};
}

