#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glad/glad.h>

#include "glu/BlellochScan.hpp"
#include "util/Random.hpp"

using namespace glu;

TEST_CASE("BlellochScan-simple", "[.]")
{
    const std::vector<GLuint> data{1, 2, 3, 4, 5, 6, 7, 8};

    ShaderStorageBuffer buffer(data);

    printf("Input:\n");
    print_buffer<GLuint>(buffer);

    BlellochScan blelloch_scan(DataType_Uint);

    printf("Output:\n");
    blelloch_scan(buffer.handle(), data.size());
    print_buffer<GLuint>(buffer);
}

TEST_CASE("BlellochScan-multiple-sizes")
{
    const uint64_t k_seed = 123;
    const size_t k_num_elements =
        GENERATE(1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576);

    Random random(k_seed);

    std::vector<GLuint> data = random.sample_int_vector<GLuint>(k_num_elements, 0, 100);

    ShaderStorageBuffer buffer(data);

    BlellochScan blelloch_scan(DataType_Uint);
    blelloch_scan(buffer.handle(), data.size());

    std::vector<GLuint> expected(k_num_elements);
    std::exclusive_scan(data.begin(), data.end(), expected.begin(), 0);
    REQUIRE(buffer.get_data<GLuint>() == expected);
}

TEST_CASE("BlellochScan-multiple-partitions")
{
    const uint64_t k_seed = 123;
    const size_t k_num_elements = 1024;
    const size_t k_num_partitions = GENERATE(1, 32, 100, 1000);

    Random random(k_seed);

    // Generate a random buffer containing data for all partitions
    std::vector<GLuint> data = random.sample_int_vector<GLuint>(k_num_elements * k_num_partitions, 0, 100);

    ShaderStorageBuffer buffer(data);

    // Run blelloch scan on all partitions
    BlellochScan blelloch_scan(DataType_Uint);
    blelloch_scan(buffer.handle(), k_num_elements, k_num_partitions);

    // Get the result host-side
    std::vector<GLuint> result = buffer.get_data<GLuint>();

    auto data_begin = data.begin();
    auto result_begin = result.begin();

    // Check that exclusive scan was run for every partition
    for (int partition = 0; partition < k_num_partitions; partition++)
    {
        std::vector<GLuint> expected_result(k_num_elements);
        std::exclusive_scan(data_begin, data_begin + k_num_elements, expected_result.begin(), 0);

        REQUIRE(std::memcmp(expected_result.data(), &(*result_begin), k_num_elements * sizeof(GLuint)) == 0);

        data_begin += k_num_elements;
        result_begin += k_num_elements;
    }
}
