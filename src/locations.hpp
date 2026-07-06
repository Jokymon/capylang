#pragma once
#include <string>

struct source_position
{
    std::string filename = "";
    size_t line = 1;
    size_t column = 1;
};

struct source_range
{
    source_position start;
    source_position end;
};

template <typename T>
struct located
{
    T value;
    source_range location;
};
