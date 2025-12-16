#include "wat_generator.h"

void wat_generator::generate(wasm_module& module, std::ostream &output)
{
    module.dump(output);
}