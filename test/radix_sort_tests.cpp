#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glad/glad.h>
#include <cinttypes>

#include "RadixSort.hpp"
#include "util/Random.hpp"
#include "util/misc_utils.hpp"

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
            if (!inserted) iterator->second++;
        }
        return histogram;
    }

    /// Checks that vector1 is a permutation of vector2.
    template<typename T>
    void check_permutation(const std::vector<T>& vector1, const std::vector<T>& vector2)
    {
        CHECK(vector1.size() == vector2.size());

        auto histogram1 = build_value_histogram(vector1);
        auto histogram2 = build_value_histogram(vector2);
        CHECK(histogram1 == histogram2);
    }

    /// Checks whether the given vector is sorted.
    template<typename T>
    void check_sorted(const std::vector<T>& vector)
    {
        CAPTURE(vector);
        CHECK(std::is_sorted(vector.begin(), vector.end()));
    }
}

TEST_CASE("RadixSort-simple", "[.]")
{
    const size_t k_num_elements = 174;//GENERATE(range(100, 200));

    const uint64_t k_seed = 1;//GENERATE(range(0, 10));
    Random random(k_seed);

    printf("Num elements: %zu; Seed: %" PRIu64 "\n", k_num_elements, k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(k_num_elements, 0, 10);
    std::vector<GLuint> vals(k_num_elements);

    REQUIRE(keys.size() == vals.size());

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

//    printf("Input; Key buffer:\n");
//    print_buffer<GLuint>(key_buffer);
//    print_buffer_hex(key_buffer);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

//    printf("Output; Key buffer:\n");
//    print_buffer<GLuint>(key_buffer);
//    print_buffer_hex(key_buffer);

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}

TEST_CASE("RadixSort-1024", "[.]")
{
    const uint64_t k_seed = 123;
    Random random(k_seed);

    std::vector<GLuint> keys = random.sample_int_vector<GLuint>(1024, 0, 100);
    std::vector<GLuint> vals = random.sample_int_vector<GLuint>(1024, 100, 200);

    ShaderStorageBuffer key_buffer(keys);
    ShaderStorageBuffer val_buffer(vals);

    RadixSort radix_sort;
    radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());

    std::vector<GLuint> sorted_keys = key_buffer.get_data<GLuint>();

    check_permutation(keys, sorted_keys);
    check_sorted(sorted_keys);
}