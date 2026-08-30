#pragma once
#include <map>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

enum class wasm_type
{
    none,

    u8,
    u16,
    u32,
    u64,

    i8,
    i16,
    i32,
    i64,

    f32,
    f64,
};

enum class wasm_extern_index
{
    funcidx,
    tableidx,
    memidx,
    globalidx,
    tagidx
};

using index_type = uint32_t;

struct argument_type
{
    std::string name;
    wasm_type type;
};
using arguments_type = std::vector<argument_type>;

enum class wasm_op
{
    nop,
    unreachable,
    drop,

    local_get,
    local_set,
    local_tee,
    global_get,
    global_set,

    load,
    store,

    iadd,
    isub,
    imul,
    idiv,
    irem,
    ishl,
    ishr,
    iand,
    ior,

    eq,
    ne,
    eqz,
    ilt,
    ilte,
    igt,
    igte,

    br,
    br_if,
    call,
    ret,

    memory_size,
    memory_grow,
    memory_fill,

    typ_const
};

enum class access_type
{
    mut,
    immut
};

class wasm_branch_label
{
public:
    explicit wasm_branch_label();
    std::string repr() const;

    bool operator==(const wasm_branch_label& other);

private:
    static int label_index;

private:
    std::string label_representation;
};

class wasm_function_ref
{
public:
    explicit wasm_function_ref(const std::string& name);
    std::string name() const;

private:
    std::string name_;
};

struct wasm_op_base;
struct wasm_block;
struct wasm_if_block;
struct wasm_op_index;
struct wasm_op_type;
struct wasm_op_type_sign;
struct wasm_op_type_value;
struct wasm_op_align_offset;
struct wasm_op_label;
struct wasm_op_func;
// wasm_instruction roughly maps to the 'instr' element in the WASM spec
using wasm_instruction = std::variant<
    wasm_op_base,
    wasm_block,
    wasm_if_block,
    wasm_op_index,
    wasm_op_type,
    wasm_op_type_sign,
    wasm_op_type_value,
    wasm_op_align_offset,
    wasm_op_label,
    wasm_op_func
>;
// wasm_expr roughly maps to the 'expr' element in the WASM spec
using wasm_expr = std::vector<std::unique_ptr<wasm_instruction>>;

struct wasm_op_base
{
    explicit wasm_op_base(wasm_op op);

    wasm_op op;
};

struct wasm_op_index : public wasm_op_base
{
    explicit wasm_op_index(wasm_op op, const std::string var_name);

    std::string name;
};

struct wasm_op_type : public wasm_op_base
{
    explicit wasm_op_type(wasm_op op, wasm_type type);

    wasm_type value_type;
};

struct wasm_op_type_sign : public wasm_op_base
{
    explicit wasm_op_type_sign(wasm_op op, wasm_type type);

    wasm_type value_type;
};

struct wasm_op_type_value : public wasm_op_base
{
    explicit wasm_op_type_value(wasm_op op, wasm_type type, int64_t value);

    wasm_type value_type;
    int64_t value;
};

struct wasm_op_align_offset : public wasm_op_base
{
    explicit wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset);

    wasm_type value_type;
    uint32_t alignment;
    uint64_t offset;
};

struct wasm_op_label : public wasm_op_base
{
    explicit wasm_op_label(wasm_op op, wasm_branch_label label);

    wasm_branch_label label;
};

struct wasm_op_func : public wasm_op_base
{
    explicit wasm_op_func(wasm_op op, wasm_function_ref function);

    wasm_function_ref function;
};

struct wasm_if_block
{
    explicit wasm_if_block(wasm_type return_type, wasm_block* enclosing_block);
    std::pair<wasm_block&, wasm_block&> blocks();

    wasm_type return_type;
    std::unique_ptr<wasm_block> then_block;
    std::unique_ptr<wasm_block> else_block;
};

namespace wasm_builder
{
    wasm_block& block(wasm_expr& expr, wasm_block* parent, wasm_type return_type);
    wasm_block& loop(wasm_expr& expr, wasm_block* parent, wasm_type return_type);
    std::pair<wasm_block&, wasm_block&> if_block(wasm_expr& expr, wasm_block* parent, wasm_type return_type);

    void local_get(wasm_expr& expr, const char* variable_name);
    void local_set(wasm_expr& expr, const char* variable_name);
    void global_get(wasm_expr& expr, const char* variable_name);
    void global_set(wasm_expr& expr, const char* variable_name);
    void const_val(wasm_expr& expr, wasm_type type, int64_t value);
    void drop(wasm_expr& expr);
    void add(wasm_expr& expr, wasm_type type);
    void and_instr(wasm_expr& expr, wasm_type type);
    void div(wasm_expr& expr, wasm_type type);
    void mod(wasm_expr& expr, wasm_type type);
    void mul(wasm_expr& expr, wasm_type type);
    void or_instr(wasm_expr& expr, wasm_type type);
    void sub(wasm_expr& expr, wasm_type type);
    void shl(wasm_expr& expr, wasm_type type);
    void shr(wasm_expr& expr, wasm_type type);
    void load(wasm_expr& expr, wasm_type type, size_t offset = 0);
    void store(wasm_expr& expr, wasm_type type, size_t offset = 0);
    void eq(wasm_expr& expr, wasm_type type);
    void ne(wasm_expr& expr, wasm_type type);
    void eqz(wasm_expr& expr, wasm_type type);
    void lt(wasm_expr& expr, wasm_type type);
    void lte(wasm_expr& expr, wasm_type type);
    void gt(wasm_expr& expr, wasm_type type);
    void gte(wasm_expr& expr, wasm_type type);
    void br(wasm_expr& expr, wasm_branch_label branch_label);
    void br_if(wasm_expr& expr, wasm_branch_label branch_label);
    void call(wasm_expr& expr, const char* function_name);
    void ret(wasm_expr& expr);
    void memory_size(wasm_expr& expr);
    void memory_grow(wasm_expr& expr);
    void memory_fill(wasm_expr& expr);
    void unreachable(wasm_expr& expr);
}

struct wasm_block
{
public:
    enum wasm_block_type
    {
        t_plain, // used for function bodies and then/else-blocks
        t_block, // used for typed non-loop blocks
        t_loop   // used for typed loop-blocks
    };

public:
    size_t inst_count() const;
    wasm_branch_label label() const;

    wasm_block_type block_type;
    wasm_type return_type;

    wasm_branch_label block_label;
    wasm_block* enclosing_block = nullptr;

    wasm_expr instructions;
};

struct exportable
{
public:
    explicit exportable(index_type index, wasm_extern_index export_type);

    std::string name;
    index_type index;
    wasm_extern_index export_type;
};

struct importable
{
public:
    void import_from(const char* ns, const char* import_name);
    bool is_imported() const;

    std::string import_name;
    std::string ns;
};

struct wasm_export
{
    std::string export_name;
    std::string name;
    index_type index;
    wasm_extern_index export_type;
};

struct wasm_function : public exportable, public importable
{
public:
    void allocate_local(const char* name, wasm_type var_type);

    // prefer to use the create..() function of the module
    explicit wasm_function(index_type index, const std::string& name, wasm_type return_type, arguments_type arguments);

    wasm_type return_type;
    // arguments only hold the definitions of the functions arguments in index order
    arguments_type arguments;
    // locals only holds the definitions of the functions local variables in index order
    arguments_type locals;
    // the locals_map maps both, argument names and local variable names to indices
    std::map<std::string, size_t> locals_map;
    wasm_expr function_body;
};

struct wasm_data_section
{
    explicit wasm_data_section(size_t offset);

    size_t allocate_data(const std::string& data);

    size_t init_offset;
    size_t offset;
    std::string data_buffer;
};

struct wasm_memory : public exportable, public importable
{
    explicit wasm_memory(index_type index, size_t initial_block_count);

    size_t initial_block_count;
    size_t max_block_count = 0;
};

struct wasm_global
{
    std::string name;
    access_type access;
    wasm_type typ;
    wasm_expr init_expression;
};

struct wasm_module
{
    explicit wasm_module();

    wasm_memory& create_memory(size_t initial_block_count);
    wasm_data_section& create_data_section(size_t init_offset);
    wasm_global& create_global(const char* name, wasm_type g_type, access_type access);
    wasm_function& create_function(const char* name, wasm_type return_type, arguments_type arguments);

    void append_data_section(wasm_data_section& data);

    void export_as(const char* export_name, exportable& object);

    // vectors with WASM items where the index in the vector represents the
    // index in corresponding WASM section
    std::vector<wasm_memory> memories;
    std::vector<wasm_data_section> data_sections;
    std::vector<wasm_global> globals;
    std::map<std::string, size_t> globals_map;
    std::vector<wasm_export> exports;
    std::vector<wasm_function> functions;
    std::map<std::string, size_t> functions_map;
};
