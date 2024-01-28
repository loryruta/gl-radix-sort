#ifndef GLU_GL_UTILS_HPP
#define GLU_GL_UTILS_HPP

#include <cmath>
#include <string>
#include <vector>

#include "errors.hpp"

namespace glu
{
    inline void
    copy_buffer(GLuint src_buffer, GLuint dst_buffer, size_t size, size_t src_offset = 0, size_t dst_offset = 0)
    {
        glBindBuffer(GL_COPY_READ_BUFFER, src_buffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, dst_buffer);

        glCopyBufferSubData(
            GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr) src_offset, (GLintptr) dst_offset, (GLsizeiptr) size
        );
    }

    /// A RAII wrapper for GL shader.
    class Shader
    {
    private:
        GLuint m_handle;

    public:
        explicit Shader(GLenum type) :
            m_handle(glCreateShader(type)){};
        Shader(const Shader&) = delete;

        Shader(Shader&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = 0;
        }

        ~Shader() { glDeleteShader(m_handle); }

        [[nodiscard]] GLuint handle() const { return m_handle; }

        void source_from_str(const std::string& src_str)
        {
            const char* src_ptr = src_str.c_str();
            glShaderSource(m_handle, 1, &src_ptr, nullptr);
        }

        void source_from_file(const char* src_filepath)
        {
            FILE* file = fopen(src_filepath, "rt");
            GLU_CHECK_STATE(!file, "Failed to shader file: %s", src_filepath);

            fseek(file, 0, SEEK_END);
            size_t file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            std::string src{};
            src.resize(file_size);
            fread(src.data(), sizeof(char), file_size, file);
            source_from_str(src.c_str());

            fclose(file);
        }

        std::string get_info_log()
        {
            GLint log_length = 0;
            glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> log(log_length);
            glGetShaderInfoLog(m_handle, log_length, nullptr, log.data());
            return {log.begin(), log.end()};
        }

        void compile()
        {
            glCompileShader(m_handle);

            GLint status;
            glGetShaderiv(m_handle, GL_COMPILE_STATUS, &status);
            if (!status)
            {
                GLU_CHECK_STATE(status, "Shader failed to compile: %s", get_info_log().c_str());
            }
        }
    };

    /// A RAII wrapper for GL program.
    class Program
    {
    private:
        GLuint m_handle;

    public:
        explicit Program() { m_handle = glCreateProgram(); };
        Program(const Program&) = delete;

        Program(Program&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = 0;
        }

        ~Program() { glDeleteProgram(m_handle); }

        [[nodiscard]] GLuint handle() const { return m_handle; }

        void attach_shader(GLuint shader_handle) { glAttachShader(m_handle, shader_handle); }
        void attach_shader(const Shader& shader) { glAttachShader(m_handle, shader.handle()); }

        [[nodiscard]] std::string get_info_log() const
        {
            GLint log_length = 0;
            glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> log(log_length);
            glGetProgramInfoLog(m_handle, log_length, nullptr, log.data());
            return {log.begin(), log.end()};
        }

        void link()
        {
            GLint status;
            glLinkProgram(m_handle);
            glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
            if (!status)
            {
                GLU_CHECK_STATE(status, "Program failed to link: %s", get_info_log().c_str());
            }
        }

        void use() { glUseProgram(m_handle); }

        GLint get_uniform_location(const char* uniform_name)
        {
            GLint loc = glGetUniformLocation(m_handle, uniform_name);
            GLU_CHECK_STATE(loc >= 0, "Failed to get uniform location: %s", uniform_name);
            return loc;
        }
    };

    /// A RAII helper class for GL shader storage buffer.
    class ShaderStorageBuffer
    {
    private:
        GLuint m_handle = 0;
        size_t m_size = 0;

    public:
        explicit ShaderStorageBuffer(size_t initial_size = 0)
        {
            if (initial_size > 0)
                resize(initial_size, false);
        }

        explicit ShaderStorageBuffer(const void* data, size_t size) :
            m_size(size)
        {
            GLU_CHECK_ARGUMENT(data, "");
            GLU_CHECK_ARGUMENT(size > 0, "");

            glCreateBuffers(1, &m_handle);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) m_size, data, GL_DYNAMIC_STORAGE_BIT);
        }

        template<typename T>
        explicit ShaderStorageBuffer(const std::vector<T>& data) :
            ShaderStorageBuffer(data.data(), data.size() * sizeof(T))
        {
        }

        ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
        ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
        {
            m_handle = other.m_handle;
            m_size = other.m_size;
            other.m_handle = 0;
        }

        ~ShaderStorageBuffer()
        {
            if (m_handle)
                glDeleteBuffers(1, &m_handle);
        }

        [[nodiscard]] GLuint handle() const { return m_handle; }
        [[nodiscard]] size_t size() const { return m_size; }

        /// Grows or shrinks the buffer. If keep_data, performs an additional copy to maintain the data.
        void resize(size_t size, bool keep_data = false)
        {
            size_t old_size = m_size;
            GLuint old_handle = m_handle;

            if (old_size != size)
            {
                m_size = size;

                glCreateBuffers(1, &m_handle);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
                glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) m_size, nullptr, GL_DYNAMIC_STORAGE_BIT);

                if (keep_data)
                    copy_buffer(old_handle, m_handle, std::min(old_size, size));

                glDeleteBuffers(1, &old_handle);
            }
        }

        /// Clears the entire buffer with the given GLuint value (repeated).
        void clear(GLuint value)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &value);
        }

        void write_data(const void* data, size_t size)
        {
            GLU_CHECK_ARGUMENT(size <= m_size, "");

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        }

        template<typename T>
        std::vector<T> get_data() const
        {
            GLU_CHECK_ARGUMENT(m_size % sizeof(T) == 0, "Size %zu isn't a multiple of %zu", m_size, sizeof(T));

            std::vector<T> result(m_size / sizeof(T));
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) m_size, result.data());
            return result;
        }
    };

    template<typename IntegerT>
    IntegerT log32_floor(IntegerT n)
    {
        return (IntegerT) floor(double(log2(n)) / 5.0);
    }

    template<typename IntegerT>
    IntegerT log32_ceil(IntegerT n)
    {
        return (IntegerT) ceil(double(log2(n)) / 5.0);
    }

    template<typename IntegerT>
    IntegerT div_ceil(IntegerT n, IntegerT d)
    {
        return (IntegerT) ceil(double(n) / double(d));
    }

    template<typename T>
    bool is_power_of_2(T n)
    {
        return (n & (n - 1)) == 0;
    }

    template<typename IntegerT>
    IntegerT next_power_of_2(IntegerT n)
    {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }

    template<typename T>
    void print_buffer(const ShaderStorageBuffer& buffer)
    {
        std::vector<T> data = buffer.get_data<T>();

        std::string entry_str;
        for (size_t i = 0; i < data.size(); i++)
        {
            printf("(%zu) %s, ", i, std::to_string(data.at(i)).c_str());
        }
        printf("\n");
    }

    inline void print_buffer_hex(const ShaderStorageBuffer& buffer)
    {
        std::vector<GLuint> data = buffer.get_data<GLuint>();
        for (size_t i = 0; i < data.size(); i++)
            printf("(%zu) %08x, ", i, data[i]);
        printf("\n");
    }
} // namespace glu

#endif // GLU_GL_UTILS_HPP
