#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "glu/Reduce.hpp"
#include "util/Random.hpp"
#include "util/StopWatch.hpp"

using namespace glu;
using namespace Catch::Matchers;

TEST_CASE("Reduce-simple-uint")
{
    const uint32_t k_data[]{32, 35, 1,  3,  95, 10, 22, 24, 44, 37, 7,  80, 33, 54, 46, 23, 14, 84, 11, 67,
                            4,  58, 70, 61, 16, 36, 83, 9,  56, 99, 28, 98, 69, 21, 51, 34, 48, 91, 62, 19,
                            59, 79, 39, 92, 97, 78, 52, 40, 66, 47, 89, 88, 74, 49, 31, 20, 45, 13, 26, 72,
                            43, 30, 65, 94, 63, 8,  60, 15, 93, 86, 41, 75, 12, 73, 55, 90, 64, 96, 53, 1,
                            57, 71, 50, 42, 29, 2,  77, 25, 82, 18, 81, 85, 27, 5,  6,  68, 17, 38, 87, 76};
    const size_t k_data_length = std::size(k_data);

    ShaderStorageBuffer buffer(k_data, k_data_length * sizeof(uint32_t));

    SECTION("sum")
    {
        Reduce reduce(DataType_Uint, ReduceOperator_Sum);
        reduce(buffer.handle(), k_data_length);
        CHECK(buffer.get_data<uint32_t>()[0] == 4951);
    }

    SECTION("mul")
    {
        Reduce reduce(DataType_Uint, ReduceOperator_Mul);
        reduce(buffer.handle(), 5);
        CHECK(buffer.get_data<uint32_t>()[0] == 319200);
    }

    SECTION("min")
    {
        Reduce reduce(DataType_Uint, ReduceOperator_Min);
        reduce(buffer.handle(), k_data_length);
        CHECK(buffer.get_data<uint32_t>()[0] == 1);
    }

    SECTION("max")
    {
        Reduce reduce(DataType_Uint, ReduceOperator_Max);
        reduce(buffer.handle(), k_data_length);
        CHECK(buffer.get_data<uint32_t>()[0] == 99);
    }
}

TEST_CASE("Reduce-all")
{
    SECTION("uint")
    {
        const std::vector<uint32_t> k_data{1, 11, 80, 73, 48, 40, 89, 36, 70, 57};
        Reduce reduce(DataType_Uint, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());
        CHECK(buffer.get_data<uint32_t>()[0] == 505);
    }

    SECTION("float")
    {
        const std::vector<float> k_data{42.138f, 18.228f, -19.127f, 86.564f,  11.904f,
                                        48.538f, 30.606f, 11.338f,  -32.699f, -29.587f};
        Reduce reduce(DataType_Float, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());
        CHECK_THAT(buffer.get_data<float>()[0], WithinAbs(167.9f, 0.1f));
    }

    SECTION("double")
    {
        const std::vector<double> k_data{-6.20, -56.02, 49.42, 52.38, -23.81, -29.72, 95.46, 77.37, -85.00, 81.74};
        Reduce reduce(DataType_Double, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());
        CHECK_THAT(buffer.get_data<double>()[0], WithinAbs(155.6, 0.1));
    }

    SECTION("vec2")
    {
        const std::vector<glm::vec2> k_data{{-77.08f, 19.54f}, {98.89f, -16.09f},  {10.53f, 91.17f}, {43.06f, -94.18f},
                                            {-19.18f, 0.86f},  {-49.99f, -92.53f}, {-4.68f, 42.34f}, {2.79f, -4.26f},
                                            {-17.49f, 43.99f}, {79.45f, -14.58f}};
        Reduce reduce(DataType_Vec2, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());

        glm::vec2 sum = buffer.get_data<glm::vec2>()[0];
        CHECK_THAT(sum.x, WithinAbs(66.29f, 0.1f));
        CHECK_THAT(sum.y, WithinAbs(-23.75f, 0.1f));
    }

    SECTION("vec4")
    {
        const std::vector<glm::vec4> k_data{{-17.04f, 1.79f, 82.67f, 39.72f},    {52.66f, 24.75f, -19.05f, 91.92f},
                                            {19.15f, 44.93f, -52.13f, 18.85f},   {-84.25f, 69.53f, -11.43f, 33.17f},
                                            {19.46f, -14.30f, -15.20f, -63.83f}, {-20.51f, -56.75f, -2.70f, 82.66f},
                                            {3.86f, 55.48f, -12.37f, -11.02f},   {-30.62f, -67.54f, -29.89f, -77.30f},
                                            {-21.55f, 50.46f, 39.34f, 81.08f},   {-56.40f, 84.61f, 90.26f, 13.35f}};
        Reduce reduce(DataType_Vec4, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());

        glm::vec4 sum = buffer.get_data<glm::vec4>()[0];
        CHECK_THAT(sum.x, WithinAbs(-135.24f, 0.1f));
        CHECK_THAT(sum.y, WithinAbs(192.97f, 0.1f));
        CHECK_THAT(sum.z, WithinAbs(69.49f, 0.1f));
        CHECK_THAT(sum.w, WithinAbs(208.59f, 0.1f));
    }

    SECTION("ivec2")
    {
        const std::vector<glm::ivec2> k_data{{-38, -88}, {57, -34}, {61, 60},  {-90, 73}, {-23, -17},
                                             {34, -79},  {-80, 53}, {24, -23}, {-88, 69}, {-83, -67}};
        Reduce reduce(DataType_IVec2, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());

        glm::ivec2 sum = buffer.get_data<glm::ivec2>()[0];
        CHECK(sum.x == -226);
        CHECK(sum.y == -53);
    }

    SECTION("ivec4")
    {
        const std::vector<glm::ivec4> k_data{{-95, 99, -30, 2},   {-69, 33, 78, 20},  {33, -43, -38, -26},
                                             {69, -67, -17, -57}, {18, -23, -2, -53}, {88, -96, 40, -48},
                                             {-93, -47, -91, 59}, {-89, 82, 10, 94},  {-15, 7, 41, 14},
                                             {63, 53, -40, 53}};
        Reduce reduce(DataType_IVec4, ReduceOperator_Sum);
        ShaderStorageBuffer buffer(k_data);
        reduce(buffer.handle(), k_data.size());

        glm::ivec4 sum = buffer.get_data<glm::ivec4>()[0];
        CHECK(sum.x == -90);
        CHECK(sum.y == -2);
        CHECK(sum.z == -49);
        CHECK(sum.w == 58);
    }
}

TEST_CASE("Reduce-subgroup-fitting-size")
{
    const size_t k_num_elements = GENERATE(32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072);

    const uint64_t k_seed = 1;
    Random random(k_seed);

    std::vector<GLuint> data = random.sample_int_vector<GLuint>(k_num_elements, 0, 100);
    GLuint sum = std::accumulate(data.begin(), data.end(), GLuint(0));

    ShaderStorageBuffer buffer(data.data(), data.size() * sizeof(uint32_t));

    Reduce reduce(DataType_Uint, ReduceOperator_Sum);
    reduce(buffer.handle(), data.size());

    uint32_t calc_sum = buffer.get_data<uint32_t>()[0];
    CHECK(calc_sum == sum);
}

TEST_CASE("Reduce-subgroup-non-fitting-size")
{
    const size_t k_num_elements = GENERATE(1, 31, 93, 201, 693, 2087, 7358, 88289, 345897, 6094798, 5238082, 10043898);

    const uint64_t k_seed = 1;
    Random random(k_seed);

    std::vector<GLuint> data = random.sample_int_vector<GLuint>(k_num_elements, 0, 100);
    GLuint sum = std::accumulate(data.begin(), data.end(), GLuint(0));

    ShaderStorageBuffer buffer(data.data(), data.size() * sizeof(GLuint));

    Reduce reduce(DataType_Uint, ReduceOperator_Sum);
    reduce(buffer.handle(), data.size());

    GLuint calc_sum = buffer.get_data<GLuint>()[0];
    CHECK(calc_sum == sum);
}

TEST_CASE("Reduce-benchmark", "[.][benchmark]")
{
    const size_t k_num_elements = GENERATE(
        1024,      // 1KB
        16384,     // 16KB
        65536,     // 65KB
        131072,    // 131KB
        524288,    // 524KB
        1048576,   // 1MB
        16777216,  // 16MB
        67108864,  // 67MB
        134217728, // 134MB
        268435456  // 268MB
    );

    std::vector<GLuint> data(k_num_elements); // Don't need to initialize the vector for benchmarking

    ShaderStorageBuffer buffer(data);

    Reduce reduce(DataType_Uint, ReduceOperator_Sum);

    uint64_t ns = measure_gl_elapsed_time([&]() { reduce(buffer.handle(), k_num_elements); });

    printf("Reduce; Num elements: %zu, Elapsed: %s\n", k_num_elements, ns_to_human_string(ns).c_str());
}
