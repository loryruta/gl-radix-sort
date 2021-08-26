#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <numeric>
#include <chrono>
#include <algorithm>

#include "radix_sort.hpp"
#include "renderdoc.hpp"
#include "sort_test.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define RGC_RADIX_SORT_DATA_MAX_VAL 10'000
#define RGC_RADIX_SORT_ARRAY_LENGTH 30'206'700

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

/*
void print_radix_sort_debug_info(rgc::radix_sorter_debug_info debug_info)
{
	char time_buf[256];

	auto us = [](GLint ns) { return ns / 1000; };
	auto ms = [](GLint ns) { return ns / (1000 * 1000); };

	printf("--------------------------------------------------------------------------------------------------------------------------------\n");

	printf("%-18s%-32s%-32s%-32s%-32s\n",
		"Iteration",
		"Initial counting",
		"Per partition counting",
		"Per partition prefix sum",
		"Reordering"
		);

	// Per iteration phase elapsed time
	for (int i = 0; i < debug_info.m_iter_elapsed_time.size(); i++)
	{
		printf("%-18d", i);

		auto time = debug_info.m_iter_elapsed_time[i];

		for (int iter_phase = 0; iter_phase < rgc::radix_sorter_debug_info::Count; iter_phase++)
		{
			sprintf(time_buf, "%d us (%d ms)", us(time[iter_phase]), ms(time[iter_phase]));
			printf("%-32s", time_buf);
		}

		printf("\n");
	}

	// Average elapsed time
	auto time = debug_info.m_iter_avg_elapsed_time;
	printf("%-18s", "Average time:");
	for (int iter_phase = 0; iter_phase < rgc::radix_sorter_debug_info::Count; iter_phase++)
	{
		sprintf(time_buf, "%d us (%d ms)", us(time[iter_phase]), ms(time[iter_phase]));
		printf("%-32s", time_buf);
	}
	printf("\n");

	// Total sorting time
	//printf("%-*s\n", 18 + 32 * 4, "-");
	printf("--------------------------------------------------------------------------------------------------------------------------------\n");

	printf("%-18s%zu\n", "Items num:", debug_info.m_items_num);

	sprintf(time_buf, "%d us (%d ms)", us(debug_info.m_elapsed_time), ms(debug_info.m_elapsed_time));
	printf("%-18s%-32s\n", "Sorting time:", time_buf);

	printf("--------------------------------------------------------------------------------------------------------------------------------\n");
}
*/

void load_or_generate_array(std::filesystem::path const& path, size_t arr_len, std::vector<GLuint>& res)
{
	res.resize(arr_len);
	size_t i = 0;

	std::ifstream in_f(path);
	if (in_f.is_open())
	{
		int n;
		while (in_f >> n && i < arr_len)
		{
			res[i] = n;
			i++;
		}
	}

	if (i < arr_len)
	{
		std::random_device random_dev;
		std::mt19937 random_gen(random_dev());
		std::uniform_int_distribution<GLuint> val_dist(0, RGC_RADIX_SORT_DATA_MAX_VAL);
		for (; i < arr_len; i++) {
			res[i] = val_dist(random_gen);
		}
	}
}

void save_array(std::filesystem::path const& path, std::vector<GLuint> const& arr)
{
	std::ofstream out_f(path);
	if (!out_f.is_open()) {
		throw std::runtime_error("Couldn't open file: .random_data");
	}

	for (size_t i = 0; i < arr.size(); i++) {
		out_f << arr[i] << "\n";
	}
}

#define ARR_COMPARE_ITEMS_NUM   4
#define ARR_COMPARE_THREADS_NUM 64

void run_app()
{
	// Array creation
	printf("Creation of the array...\n");
	fflush(stdout);

	std::random_device random_device;
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<GLuint> distribution(0u, ~0u);

	std::vector<GLuint> arr(RGC_RADIX_SORT_ARRAY_LENGTH);
	for (size_t i = 0; i < arr.size(); i++) {
		arr[i] = distribution(generator);
	}

	//load_or_generate_array(".random_data", RGC_RADIX_SORT_ARRAY_LENGTH, arr);
	//save_array(".random_data", arr);

	GLuint arr_buf;
	glGenBuffers(1, &arr_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, arr_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizei) (RGC_RADIX_SORT_ARRAY_LENGTH * sizeof(GLuint)), arr.data(), GL_DYNAMIC_STORAGE_BIT);

	// Query creation
	GLuint query;
	glGenQueries(1, &query);

	// Debug tools creation
	rgc::sort_test::check_permuted check_permuted(RGC_RADIX_SORT_DATA_MAX_VAL);
	rgc::sort_test::check_sorted check_sorted;

	// Radix sort
	auto radix_sorter = std::make_unique<rgc::radix_sorter>(RGC_RADIX_SORT_ARRAY_LENGTH);

	check_permuted.memorize_original_array(arr_buf, RGC_RADIX_SORT_ARRAY_LENGTH);

	// Sort
	printf("GPU sorting...\n");
	fflush(stdout);

	glBeginQuery(GL_TIME_ELAPSED, query);
	radix_sorter->sort(arr_buf, NULL, RGC_RADIX_SORT_ARRAY_LENGTH);
	glEndQuery(GL_TIME_ELAPSED);

	GLuint elapsed_time;
	glGetQueryObjectuiv(query, GL_QUERY_RESULT, &elapsed_time);

	printf("Array length: %d - %d us (%d ms)\n",
		RGC_RADIX_SORT_ARRAY_LENGTH,
		elapsed_time / 1000,
		elapsed_time / (1000 * 1000)
	);
	fflush(stdout);

	// Save sorted array
	/*
	std::vector<GLuint> sorted_arr(arr.size());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, arr_buf);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) (RGC_RADIX_SORT_ARRAY_LENGTH * sizeof(GLuint)), sorted_arr.data());
	save_array(".sorted_data", sorted_arr);*/

	// Check permutation
	printf("Checking permutation...\n");
	fflush(stdout);

	check_permuted.memorize_permuted_array(arr_buf, RGC_RADIX_SORT_ARRAY_LENGTH);

	if (!check_permuted.is_permuted())
	{
		rgc::renderdoc::watch(false, [&]() { check_permuted.is_permuted(); });

		fprintf(stderr, "Sorting went wrong: elements are not permuted.\n");
		fflush(stderr);

		return;
	}

	// Check sorted
	printf("Checking sorting...\n");
	fflush(stdout);

	GLuint errors_count;
	if (!check_sorted.is_sorted(arr_buf, RGC_RADIX_SORT_ARRAY_LENGTH, &errors_count))
	{
		fprintf(stderr, "Sorting went wrong: array not sorted. Number of errors: %d.\n", errors_count);
		fflush(stderr);

		return;
	}
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

void print_gl_debug()
{
	GLint v1, v2, v3;

	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("Shading language version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

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
}

int main()
{
	_set_printf_count_output(true);

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

	//print_gl_debug();
	//fflush(stdout);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

	run_app();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
