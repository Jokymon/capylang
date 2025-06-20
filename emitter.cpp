#include "emitter.hpp"

emitter::emitter(std::ostream &output) : output_(output)
{
}

void emitter::generate(const ast_node &node)
{
    output_ << "(module\n";
    output_ << "  (type (;0;) (func))\n";
    output_ << "  (type (;1;) (func (param i32)))\n";
    output_ << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
    output_ << "  (memory (;0;) 2)\n";
    output_ << "  (export \"memory\" (memory 0))\n";
    output_ << "  (export \"_start\" (func $_start))\n";
    output_ << "  (func $_start (type 0)\n";

    this->emit(node);

    output_ << "      call $__imported_wasi_snapshot_preview1_proc_exit\n";
    output_ << "      unreachable\n";
    output_ << "  )\n";
    output_ << ")\n";
}

void emitter::emit(const ast_node &node)
{
    std::visit([&, this](const auto &n)
               {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, node_number>) {
            this->emit(n);
        } else if constexpr (std::is_same_v<T, node_expression>) {
            this->emit(n);
        } else {} }, node);
}

void emitter::emit(const node_expression &root)
{
    emit(*root.left);
    emit(*root.right);
    switch (root.operation)
    {
    case token_operator::op_plus:
        output_ << "      i32.add\n";
        break;
    case token_operator::op_multiply:
        output_ << "      i32.mul\n";
        break;
    }
}

void emitter::emit(const node_number &number)
{
    output_ << "      i32.const " << number.number << "\n";
}
