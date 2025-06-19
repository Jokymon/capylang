#include "parser.hpp"
#include <ostream>

class emitter
{
public:
    explicit emitter(std::ostream &output);

    void emit(const node_root_type& root);

private:
    std::ostream &output_;
};