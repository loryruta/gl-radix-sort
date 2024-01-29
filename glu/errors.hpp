#ifndef GLU_ERRORS_HPP
#define GLU_ERRORS_HPP

#include <cstdio>
#include <cstdlib>

// TODO mark if (!condition_) as unlikely
#define GLU_CHECK_STATE(condition_, ...)                                                                                   \
    {                                                                                                                  \
        if (!(condition_))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, __VA_ARGS__);                                                                              \
            exit(1);                                                                                                   \
        }                                                                                                              \
    }

#define GLU_CHECK_ARGUMENT(condition_, ...) GLU_CHECK_STATE(condition_, __VA_ARGS__)
#define GLU_FAIL(...) GLU_CHECK_STATE(false, __VA_ARGS__)

#endif
