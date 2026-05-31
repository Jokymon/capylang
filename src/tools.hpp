#pragma once
#include <cassert>
#include <sstream>
#include <string>

#define CAPY_ASSERT(cond, msg)                                           \
    {                                                                    \
        if (!(cond))                                                     \
        {                                                                \
            printf("Failure in %s:%u: %s\n", __FILE__, __LINE__, (msg)); \
            assert((cond));                                              \
        }                                                                \
    }

#define CAPY_FAIL(msg)                                           \
    printf("Failure in %s:%u: %s\n", __FILE__, __LINE__, (msg)); \
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