#include "parser.hpp"
#include <ostream>

class emitter
{
public:
    explicit emitter(std::ostream &output);

    void emit(const ast_node& node);

    void emit(const node_expression& root);
    void emit(const node_number& number);

private:
    std::ostream &output_;
};