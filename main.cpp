#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "(module\n";
    std::cout << "  (type (;0;) (func))\n";
    std::cout << "  (type (;1;) (func (param i32)))\n";
    std::cout << "  (import \"wasi_snapshot_preview1\" \"proc_exit\" (func $__imported_wasi_snapshot_preview1_proc_exit (;0;) (type 1)))\n";
    std::cout << "  (memory (;0;) 2)\n";
    std::cout << "  (export \"memory\" (memory 0))\n";
    std::cout << "  (export \"_start\" (func $_start))\n";
    std::cout << "  (func $_start (type 0)\n";
    std::cout << "      i32.const 3\n";
    std::cout << "      call $__imported_wasi_snapshot_preview1_proc_exit\n";
    std::cout << "      unreachable\n";
    std::cout << "  )\n";
    std::cout << ")\n";

    return 0;
}
