#include <algorithm>
#include <cinttypes>
#include <unordered_map>
#include <vector>

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glad/glad.h>

#include "glu/RadixSort.hpp"
#include "util/Random.hpp"
#include "util/StopWatch.hpp"

using namespace glu;

namespace
{
    /// Builds an histogram counting the values in the given vector.
    template<typename T>
    std::unordered_map<T, size_t> build_value_histogram(const std::vector<T>& vector)
    {
        std::unordered_map<T, size_t> histogram;
        for (const T& entry : vector)
        {
            auto [iterator, inserted] = histogram.emplace(entry, 1);
            if (!inserted)
                iterator->second++;
        }
        return histogram;
    }

    /// Checks that vector1 is a permutation of vector2.
    template<typename T>
    void check_permutation(const std::vector<T>& vector1, const std::vector<T>& vector2)
    {
        CHECK(vector1.size() == vector2.size());

        std::unordered_map<T, size_t> histogram1 = build_value_histogram(vector1);
        std::unordered_map<T, size_t> histogram2 = build_value_histogram(vector2);
        CHECK(histogram1 == histogram2);
    }

    /// Checks whether the given vector is sorted.
    template<typename T>
    void check_sorted(const std::vector<T>& vector)
    {
        CAPTURE(vector);
        CHECK(std::is_sorted(vector.begin(), vector.end()));
    }
} // namespace

TEST_CASE("RadixSort-simple", "[.]")
{
    const size_t k_num_elements = 10;

    const uint64_t k_seed = 1;
    Random random(k_seed);

    printf("Num elements: %zu; Seed: %" PRIu64 "\n", k_num_elements, k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(k_num_elements, 0, UINT32_MAX);
    std::vector<GLuint> vals(k_num_elements);

    REQUIRE(keys.size() == vals.size());

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    printf("Input; Key buffer:\n");
    print_buffer<GLuint>(key_buffer);
    print_buffer_hex(key_buffer);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

    printf("Output; Key buffer:\n");
    print_buffer<GLuint>(key_buffer);
    print_buffer_hex(key_buffer);

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}

TEST_CASE("RadixSort-128-256-512-1024")
{
    const size_t k_num_elements = GENERATE(128, 256, 512, 1024);

    const uint64_t k_seed = 1;
    Random random(k_seed);

    printf("Num elements: %zu; Seed: %" PRIu64 "\n", k_num_elements, k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(k_num_elements, 0, UINT32_MAX);
    std::vector<GLuint> vals(k_num_elements);

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}

TEST_CASE("RadixSort-2048")
{
    const size_t k_num_elements = 2048;

    const uint64_t k_seed = 1;
    Random random(k_seed);

    printf("Num elements: %zu; Seed: %" PRIu64 "\n", k_num_elements, k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(k_num_elements, 0, 10);
    std::vector<GLuint> vals(k_num_elements);

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}

TEST_CASE("RadixSort-multiple-sizes")
{
    const size_t k_num_elements = GENERATE(10993, 14978, 16243, 18985, 23857, 27865, 33363, 41298, 45821, 47487);

    const uint64_t k_seed = 1;
    Random random(k_seed);

    printf("Num elements: %zu; Seed: %" PRIu64 "\n", k_num_elements, k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(k_num_elements, 0, UINT32_MAX);
    std::vector<GLuint> vals(k_num_elements);

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}

TEST_CASE("RadixSort-benchmark", "[.][benchmark]")
{
    const size_t k_num_elements = GENERATE(
        1024,      // 1KB
        16384,     // 16KB
        65536,     // 65KB
        131072,    // 131KB
        524288,    // 524KB
        1048576,   // 1MB
        2097152,   // 2MB
        4194304,   // 4MB
        8388608,   // 8MB
        16777216,  // 16MB
        33554432,  // 32MB
        67108864,  // 64MB
        134217728, // 128MB
        268435456  // 268MB
    );

    std::vector<GLuint> keys(k_num_elements); // Don't need to initialize the vector for benchmarking
    std::vector<GLuint> vals(k_num_elements);

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    RadixSort radix_sort;

    radix_sort.prepare_internal_buffers(k_num_elements);

    uint64_t ns =
        measure_gl_elapsed_time([&]() { radix_sort(key_buffer.handle(), val_buffer.handle(), k_num_elements); });

    printf("Radix sort; Num elements: %zu, Elapsed: %s\n", k_num_elements, ns_to_human_string(ns).c_str());
}
