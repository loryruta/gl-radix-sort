#include <cstdio>
#include <cstdlib>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <catch2/catch_all.hpp>

void print_gl_debug()
{
    GLint v1, v2, v3;

    printf("---------------------------------------------------------------- Device info\n");

    printf("Device: %s\n", glGetString(GL_RENDERER));
    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    printf("Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &v1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &v2);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &v3);
    printf("GL_MAX_COMPUTE_WORK_GROUP_COUNT: (%d, %d, %d)\n", v1, v2, v3);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &v1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &v2);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &v3);
    printf("GL_MAX_COMPUTE_WORK_GROUP_SIZE: (%d, %d, %d)\n", v1, v2, v3);

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &v1);
    printf("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: %d\n", v1);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &v1);
    printf("GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: %d\n", v1);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &v1);
    printf("GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS: %d\n", v1);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &v1);
    printf("GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS: %d\n", v1);

    glGetIntegerv(GL_WARP_SIZE_NV, &v1);
    printf("GL_WARP_SIZE_NV: %d\n", v1);

    printf("----------------------------------------------------------------\n");
}

void GLAPIENTRY debug_message_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam
)
{
    if (GL_DEBUG_TYPE_ERROR == type && severity <= GL_DEBUG_SEVERITY_HIGH)
    {
        fprintf(stderr, "GL CALLBACK: type = 0x%x, severity = 0x%x, message = %s\n", type, severity, message);
    }
}

int main(int argc, char* argv[])
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    if (glfwInit() == GLFW_FALSE)
    {
        fprintf(stderr, "Failed to initialize GLFW");
        exit(1);
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(500, 500, "GLU", nullptr, nullptr);
    if (window == nullptr)
    {
        fprintf(stderr, "Failed to create GLFW window");
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to load GL");
        exit(1);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_message_callback, nullptr);

    print_gl_debug();

    std::vector<const char*> catch2_args;
    for (int i = 0; i < argc; i++)
        catch2_args.emplace_back(argv[i]);

    // Show the test and sections names in stdout
    catch2_args.emplace_back("--reporter console::out=%stdout");

    int result = Catch::Session().run((int) catch2_args.size(), catch2_args.data());

    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
