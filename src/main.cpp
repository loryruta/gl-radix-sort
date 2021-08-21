#include <iostream>
#include <vector>
#include <random>
#include <numeric>
#include <chrono>
#include <algorithm>

#include "radix_sort.hpp"
#include "renderdoc.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void print_buf(GLuint key_buf, GLuint val_buf, size_t arr_size)
{
	std::vector<GLuint> key_buf_data(arr_size);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, key_buf);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(arr_size * sizeof(GLuint)), key_buf_data.data());

	for (int i = 0; i < arr_size; i++) {
		printf("%4d) %4d %08x\n", i, key_buf_data[i], key_buf_data[i]);
	}

	fflush(stdout);
}

void run_app()
{
	// ------------------------------------------------------------------------------------------------
	// Config
	// ------------------------------------------------------------------------------------------------

	size_t items_num = 10;
	size_t iter_num = 1;
	GLuint min_val = 10;
	GLuint max_val = 1000;
	size_t print_window = 512;
	bool print_before_after = true;

	// ------------------------------------------------------------------------------------------------
	// Allocation
	// ------------------------------------------------------------------------------------------------

	//std::random_device random_dev;
	//std::mt19937 gen(random_dev());
	//std::uniform_int_distribution<GLuint> distribution(min_val, max_val);

	GLuint key_buf;
	glGenBuffers(1, &key_buf);
	{
		std::vector<GLuint> data = { 342,35,234534,7,653247,3,12,22544,6,45,34667,345,23,1234,6437,1665,21324 };
		items_num = data.size();
		//std::generate(data.begin(), data.end(), [&]() {
		//	return distribution(gen);
		//});

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, key_buf);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizei(data.size() * sizeof(GLuint)), data.data(), NULL);
	}

	GLuint val_buf = NULL;
	glGenBuffers(1, &val_buf);
	{
		std::vector<GLuint> data(items_num);
		std::iota(data.begin(), data.end(), 0);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, val_buf);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, GLsizei(data.size() * sizeof(GLuint)), data.data(), NULL);
	}

	auto radix_sorter = std::make_unique<rgc::radix_sorter>(items_num);

	// ------------------------------------------------------------------------------------------------
	// Sorting
	// ------------------------------------------------------------------------------------------------

	printf("Sorting...\n");

	printf("Num of items: %zu\n", items_num);
	printf("NUm of iterations: %zu\n", iter_num);

	fflush(stdout);

	GLuint query;
	glGenQueries(1, &query);

	for (int i = 0; i < iter_num; i++)
	{
		if (print_before_after)
		{
			printf("-----------------------------------------------------------------\n");
			printf("BEFORE SORTING:\n");
			printf("-----------------------------------------------------------------\n");

			print_buf(key_buf, val_buf, (size_t) std::min<size_t>(items_num, print_window));
		}

		glBeginQuery(GL_TIME_ELAPSED, query);

		radix_sorter->sort(key_buf, val_buf, items_num);

		glEndQuery(GL_TIME_ELAPSED);

		GLint elapsed_time;
		glGetQueryObjectiv(query, GL_QUERY_RESULT, &elapsed_time);

		printf("%04d) Elapsed: %10d ns %10d µs %10d ms\n", i, elapsed_time, elapsed_time / 1000, elapsed_time / (1000 * 1000));
		fflush(stdout);

		if (print_before_after)
		{
			printf("-----------------------------------------------------------------\n");
			printf("AFTER SORTING:\n");
			printf("-----------------------------------------------------------------\n");

			print_buf(key_buf, val_buf, (size_t)std::min<size_t>(items_num, print_window));
		}
	}

	//

	//print_buf(key_buf, val_buf, num);
}

void GLAPIENTRY
MessageCallback( GLenum source,
				 GLenum type,
				 GLuint id,
				 GLenum severity,
				 GLsizei length,
				 const GLchar* message,
				 const void* userParam )
{
	if (GL_DEBUG_TYPE_ERROR == type && severity <= GL_DEBUG_SEVERITY_HIGH) {

		fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
				 ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
				 type, severity, message );
	}

}

int main()
{
	if (glfwInit() == GLFW_FALSE) {
		throw std::runtime_error("Couldn't initialize GLFW.");
	}

	GLFWwindow* window = glfwCreateWindow(500, 500, "parallel_radix_sort", nullptr, nullptr);
	if (window == nullptr) {
		throw std::runtime_error("Couldn't create GLFW window.");
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		throw std::runtime_error("Failed to initialize OpenGL context. GLAD failed to load.");
	}

	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("Shading language version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	GLint v1, v2, v3;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &v1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &v2);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &v3);
	printf("GL_MAX_COMPUTE_WORK_GROUP_COUNT: (%d, %d, %d)\n", v1, v2, v3);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &v1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &v2);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &v3);
	printf("GL_MAX_COMPUTE_WORK_GROUP_SIZE: (%d, %d, %d)\n", v1, v2, v3);

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &v1); printf("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: %d\n", v1);
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &v1);     printf("GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: %d\n", v1);
	glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &v1);  printf("GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS: %d\n", v1);
	glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &v1); printf("GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS: %d\n", v1);

	glGetIntegerv(GL_WARP_SIZE_NV, &v1); printf("GL_WARP_SIZE_NV: %d\n", v1);

	fflush(stdout);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

	run_app();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
