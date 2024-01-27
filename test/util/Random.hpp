#pragma once

#include <cmath>
#include <optional>
#include <random>

namespace glu
{
    class Random
    {
    private:
        std::optional<std::minstd_rand> m_random_engine;

    public:
        explicit Random(uint64_t seed = 0) :
            m_random_engine(seed)
        {
            m_random_engine =
                seed != 0 ? std::make_optional<std::minstd_rand>(seed) : std::make_optional<std::minstd_rand>();
        }

        ~Random() = default;

        template<typename IntegerT>
        IntegerT sample_int(IntegerT min, IntegerT max)
        {
            GLU_CHECK_ARGUMENT(min < max, "Min must be strictly lower than Max");
            return ((*m_random_engine)() % (max - min)) + min;
        }

        template<typename IntegerT>
        std::vector<IntegerT> sample_int_vector(size_t num_elements, IntegerT min, IntegerT max)
        {
            std::vector<IntegerT> result(num_elements);
            for (size_t i = 0; i < num_elements; i++)
                result[i] = sample_int(min, max);
            return result;
        }
    };
} // namespace glu
