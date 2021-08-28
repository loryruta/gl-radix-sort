#include <iostream>
#include <vector>
#include <random>
#include <numeric>

#define RGC_RADIX_SORT_DEBUG
#include "generated/radix_sort.hpp"
#include "sort_test.hpp"

#include <GLFW/glfw3.h>

#define RGC_RADIX_SORT_DATA_MAX_VAL         256
#define RGC_RADIX_SORT_MIN_ARRAY_LENGTH     100
#define RGC_RADIX_SORT_NEXT_ARRAY_LENGTH(n) (n *= 10)
#define RGC_RADIX_SORT_MAX_ARRAY_LENGTH     100'000'000
#define RGC_RADIX_SORT_AVG_ITER             3

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

void save_array(std::filesystem::path const& path, GLuint buffer, size_t buffer_length)
{
	std::vector<GLuint> data(buffer_length);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) (buffer_length * sizeof(GLuint)), data.data());

	save_array(path, data);
}

void run_app()
{
	// Random generator
	std::random_device random_device;
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<GLuint> distribution(0u, RGC_RADIX_SORT_DATA_MAX_VAL);

	// Array
	std::vector<GLuint> arr(RGC_RADIX_SORT_MAX_ARRAY_LENGTH);

	// GPU buffer creation
	GLuint key_buf;
	glGenBuffers(1, &key_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, key_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizei) (RGC_RADIX_SORT_MAX_ARRAY_LENGTH * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

	GLuint values_buf;
	glGenBuffers(1, &values_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, values_buf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizei) (RGC_RADIX_SORT_MAX_ARRAY_LENGTH * sizeof(GLuint)), nullptr, GL_DYNAMIC_STORAGE_BIT);

	// Query creation
	GLuint query;
	glGenQueries(1, &query);

	// Sorter & checkers creation
	rgc::radix_sort::sorter sorter(RGC_RADIX_SORT_MAX_ARRAY_LENGTH);
	rgc::radix_sort::check_permuted check_keys_permuted(RGC_RADIX_SORT_DATA_MAX_VAL);
	rgc::radix_sort::check_permuted check_values_permuted(RGC_RADIX_SORT_DATA_MAX_VAL);
	rgc::radix_sort::check_sorted check_sorted;

	// File creation
	std::ofstream benchmark_file("benchmark.csv");

	benchmark_file << "Number of elements, Elapsed time (µs), Elapsed time (ms)" << std::endl;

	for (size_t arr_len = RGC_RADIX_SORT_MIN_ARRAY_LENGTH; arr_len <= RGC_RADIX_SORT_MAX_ARRAY_LENGTH; RGC_RADIX_SORT_NEXT_ARRAY_LENGTH(arr_len))
	{
		printf("Array size: %zu\n", arr_len);

		GLuint avg_time_elapsed = 0;
		for (int iter = 1; iter <= RGC_RADIX_SORT_AVG_ITER; iter++)
		{
			// GPU upload
			printf("%d/%d Generating array and uploading...\n", iter, RGC_RADIX_SORT_AVG_ITER);
			fflush(stdout);

			for (size_t i = 0; i < arr_len; i++) {
				arr[i] = distribution(generator);
			}

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, key_buf);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)), arr.data());

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, values_buf);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) (arr_len * sizeof(GLuint)), arr.data());

			// Sorting
			printf("%d/%d Sorting...\n", iter, RGC_RADIX_SORT_AVG_ITER);
			fflush(stdout);

			check_keys_permuted.memorize_original_array(key_buf, arr_len);
			check_values_permuted.memorize_original_array(values_buf, arr_len);

			glBeginQuery(GL_TIME_ELAPSED, query);
			sorter.sort(key_buf, values_buf, arr_len);
			glEndQuery(GL_TIME_ELAPSED);

			check_keys_permuted.memorize_permuted_array(key_buf, arr_len);
			check_values_permuted.memorize_permuted_array(values_buf, arr_len);

			fflush(stdout);

			// Check
			printf("%d/%d Checking...\n", iter, RGC_RADIX_SORT_AVG_ITER);
			fflush(stdout);

			if (!check_keys_permuted.is_permuted())
			{
				fprintf(stderr, "Keys aren't permuted. Probably an issue with the sorting algorithm.\n");
				fflush(stderr);

				exit(1);
			}

			if (!check_values_permuted.is_permuted())
			{
				fprintf(stderr, "Values aren't permuted. Probably an issue with the sorting algorithm.\n");
				fflush(stderr);

				exit(2);
			}

			if (GLuint errors_count; !check_sorted.is_sorted(key_buf, arr_len, &errors_count))
			{
				fprintf(stderr, "Keys aren't sorted (%d inconsistencies). Probably an issue with the sorting algorithm.\n", errors_count);
				fflush(stderr);

				exit(3);
			}

			if (GLuint errors_count; !check_sorted.is_sorted(values_buf, arr_len, &errors_count)) // ONLY IF KEYS = VALUES (FOR TESTING)
			{
				fprintf(stderr, "Values aren't sorted (%d inconsistencies). Probably an issue with the sorting algorithm.\n", errors_count);
				fflush(stderr);

				exit(4);
			}

			GLuint time_elapsed; glGetQueryObjectuiv(query, GL_QUERY_RESULT, &time_elapsed);
			avg_time_elapsed += time_elapsed;
		}
		avg_time_elapsed /= RGC_RADIX_SORT_AVG_ITER;

		printf("Average time elapsed: %d us (%d ms)\n", avg_time_elapsed / 1000, avg_time_elapsed / (1000 * 1000));
		fflush(stdout);

		benchmark_file << arr_len << "," << avg_time_elapsed / 1000 << "," << (avg_time_elapsed / (1000 * 1000)) << "\n";
	}

	benchmark_file.close();
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
