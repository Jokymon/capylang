#pragma once
#include "ir.hpp"
#include <ostream>

class wat_generator
{
public:
    void generate(wasm_module& module, std::ostream &output);
};