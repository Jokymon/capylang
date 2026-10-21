#pragma once
#include "ast.hpp"
#include <cstdint>
#include <string>

// Helpers for integer literals in the AST. A literal keeps its magnitude and its
// sign separately (see ndef/ast.ndef). This way the whole range from the smallest
// s64 value up to the largest u64 value can be represented without overflows.

/// @brief Checks whether the literal lies within the range [min_value, max_value]
inline bool literal_fits(const ast::number& n, std::int64_t min_value, std::uint64_t max_value)
{
    if (!n.negative || (n.magnitude == 0))
    {
        return n.magnitude <= max_value;
    }
    if (min_value >= 0)
    {
        return false;
    }
    // magnitude of min_value; the unsigned negation also works for INT64_MIN
    const std::uint64_t min_magnitude = std::uint64_t{0} - static_cast<std::uint64_t>(min_value);
    return n.magnitude <= min_magnitude;
}

/// @brief Returns the two's complement bit pattern of the literal as a 64-bit value.
/// The interpretation (width, signedness) is defined by the type of the literal.
inline long long literal_bits(const ast::number& n)
{
    const std::uint64_t bits = n.negative ? (std::uint64_t{0} - n.magnitude) : n.magnitude;
    return static_cast<long long>(bits);
}

/// @brief Textual representation of the literal as written in the source (decimal)
inline std::string literal_repr(const ast::number& n)
{
    return (n.negative ? "-" : "") + std::to_string(n.magnitude);
}
