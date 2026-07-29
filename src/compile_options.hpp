#pragma once
#include <optional>

enum class memory_type
{
    imported,
    internal,
    exported,
    impex, // re-exporting an imported memory
};

struct compile_options
{
    size_t mem_min_blocks = 1;
    std::optional<size_t> mem_max_blocks = std::nullopt;
    memory_type mem_type = memory_type::exported;
};