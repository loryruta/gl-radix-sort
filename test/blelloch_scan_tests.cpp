#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glad/glad.h>

#include "BlellochScan.hpp"
#include "util/Random.hpp"
#include "util/misc_utils.hpp"

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
