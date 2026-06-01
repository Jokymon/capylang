#pragma once
#include <cassert>
#include <sstream>
#include <string>

#define CAPY_ASSERT(cond, msg, ...)                                                                \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("Failure in %s:%u: " #msg "\n", __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
            assert((cond));                                                                        \
        }                                                                                          \
    }

#define CAPY_FAIL(msg, ...)                                                                \
    printf("Failure in %s:%u: " #msg "\n", __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
    assert(false);

template <typename Iter>
std::string join(Iter begin, Iter end, std::string const& separator)
{
    std::ostringstream result;
    if (begin != end)
        result << *begin++;
    while (begin != end)
        result << separator << *begin++;
    return result.str();
}