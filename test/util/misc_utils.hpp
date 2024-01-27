#pragma once

#include <cmath>
#include <random>

#include <glad/glad.h>

#include "gl_utils.hpp"
#include "renderdoc.hpp"

#define RGC_SORT_TEST_THREADS_NUM 64
#define RGC_SORT_TEST_ITEMS_NUM   4

namespace glu
{
const GLuint k_zero = 0;

inline GLuint calc_workgroups_num(GLuint req_dim)
{
  return (GLuint) (ceilf(float(req_dim) / float(RGC_SORT_TEST_THREADS_NUM * RGC_SORT_TEST_ITEMS_NUM)));
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

template<typename IntegerT>
IntegerT random_int(IntegerT min, IntegerT max)
{
    GLU_CHECK_ARGUMENT(min < max, "");

    static std::minstd_rand random_engine; // TODO somewhere else
    return random_engine() % (max - min) + min;
}

}
