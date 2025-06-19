#include "emitter.hpp"

emitter::emitter(std::ostream &output) : output_(output)
{
}

void emitter::emit(const node_root_type& root) {
    output_ << "(module\n";
    output_ << "  (type (;0;) (func))\n";
    output_ << "  (type (;1;) (func (param i32)))\n";
    output_ << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
    output_ << "  (memory (;0;) 2)\n";
    output_ << "  (export \"memory\" (memory 0))\n";
    output_ << "  (export \"_start\" (func $_start))\n";
    output_ << "  (func $_start (type 0)\n";
    output_ << "      i32.const " << root.number << "\n";
    output_ << "      call $__imported_wasi_snapshot_preview1_proc_exit\n";
    output_ << "      unreachable\n";
    output_ << "  )\n";
    output_ << ")\n";
}