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

using index_type = uint32_t;

using arguments_type = std::vector<std::pair<std::string, wasm_type>>;

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

    eq,
    ne,
    eqz,

    br,
    br_if,
    call,

    typ_const
};

class wasm_branch_label
{
public:
    explicit wasm_branch_label();
    std::string repr() const;

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

struct wasm_instruction;
struct wasm_block;
struct wasm_if_block;
struct wasm_loop_block;
struct wasm_internal_block;
struct wasm_op_index;
struct wasm_op_type;
struct wasm_op_type_sign;
struct wasm_op_type_value;
struct wasm_op_align_offset;
struct wasm_op_label;
struct wasm_op_func;
using wasm_statement = std::variant<
    wasm_instruction,
    wasm_block,
    wasm_if_block,
    wasm_loop_block,
    wasm_internal_block,
    wasm_op_index,
    wasm_op_type,
    wasm_op_type_sign,
    wasm_op_type_value,
    wasm_op_align_offset,
    wasm_op_label,
    wasm_op_func
>;

struct wasm_instruction
{
    explicit wasm_instruction(wasm_op op);

    wasm_op op;
};

struct wasm_op_index : public wasm_instruction
{
    explicit wasm_op_index(wasm_op op, const std::string var_name, uint32_t index);

    std::string name;
    uint32_t index;
};

struct wasm_op_type : public wasm_instruction
{
    explicit wasm_op_type(wasm_op op, wasm_type type);

    wasm_type value_type;
};

struct wasm_op_type_sign : public wasm_instruction
{
    explicit wasm_op_type_sign(wasm_op op, wasm_type type);

    wasm_type value_type;
};

struct wasm_op_type_value : public wasm_instruction
{
    explicit wasm_op_type_value(wasm_op op, wasm_type type, uint64_t value);

    wasm_type value_type;
    uint64_t value;
};

struct wasm_op_align_offset : public wasm_instruction
{
    explicit wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset);

    wasm_type value_type;
    uint32_t alignment;
    uint64_t offset;
};

struct wasm_op_label : public wasm_instruction
{
    explicit wasm_op_label(wasm_op op, wasm_branch_label label);

    wasm_branch_label label;
};

struct wasm_op_func : public wasm_instruction
{
    explicit wasm_op_func(wasm_op op, wasm_function_ref function);

    wasm_function_ref function;
};

struct wasm_if_block
{
    explicit wasm_if_block(wasm_type return_type);
    std::pair<wasm_block&, wasm_block&> blocks();

    wasm_type return_type;
    std::unique_ptr<wasm_block> then_block;
    std::unique_ptr<wasm_block> else_block;
};

struct wasm_block
{
public:
    size_t inst_count() const;
    wasm_branch_label label() const;

    wasm_block& block(wasm_type return_type);
    wasm_block& loop(wasm_type return_type);
    std::pair<wasm_block&, wasm_block&> if_block(wasm_type return_type);

    void local_get(const char* variable_name);
    void local_get(uint32_t index);
    void local_set(const char* variable_name);
    void global_get(const char* variable_name);
    void global_set(const char* variable_name);
    void const_val(wasm_type type, uint64_t value);
    void drop();
    void add(wasm_type type);
    void div(wasm_type type);
    void mod(wasm_type type);
    void mul(wasm_type type);
    void sub(wasm_type type);
    void load(wasm_type type, size_t offset=0);
    void store(wasm_type type, size_t offset=0);
    void eq(wasm_type type);
    void ne(wasm_type type);
    void eqz(wasm_type type);
    void br(wasm_branch_label branch_label);
    void br_if(wasm_branch_label branch_label);
    void call(const char* function_name);

    wasm_branch_label block_label;

    std::vector<std::unique_ptr<wasm_statement>> instructions;
};

struct wasm_internal_block : public wasm_block
{
    explicit wasm_internal_block(wasm_type return_type);

    wasm_type return_type;
};

struct wasm_loop_block : public wasm_block
{
    explicit wasm_loop_block(wasm_type return_type);

    wasm_type return_type;
};

struct exportable
{
public:
    explicit exportable(const char* name, const char* export_type);
    explicit exportable(index_type index, const char* export_type);

    // TODO: this creates a false impression that we can just call
    // export_as on the exportable. But we still need to add the
    // element to the exports list in the module and would therefore
    // better call export_as() directly on the module without ever
    // calling this function. --> This API should be improved
    void export_as(const char* export_name);

    std::string name;
    index_type index;

    std::string export_name;
    std::string export_type;
};

struct wasm_function : public exportable
{
public:
    bool is_imported() const;

    void allocate_local(const char* name, wasm_type var_type);
    wasm_block& body();
    void import_from(const char* ns, const char* import_name);

    // prefer to use the create..() function of the module
    explicit wasm_function(const char* name, wasm_type return_type, arguments_type arguments);

    wasm_type return_type;
    arguments_type arguments;
    arguments_type locals;
    wasm_block function_body;
    std::string ns;
    std::string import_name;
};

struct wasm_data_section
{
    explicit wasm_data_section(size_t offset);

    size_t allocate_data(const std::string& data);

    size_t init_offset;
    size_t offset;
    std::string data_buffer;
};

struct wasm_memory : public exportable
{
    explicit wasm_memory(index_type index, size_t initial_block_count);

    size_t initial_block_count;
};

struct wasm_module
{
    enum access_type {
        mut, immut
    };

    explicit wasm_module();

    wasm_memory& create_memory(size_t initial_block_count);
    wasm_data_section& create_data_section(size_t init_offset);
    void create_global(const char* name, wasm_type g_type, access_type access, uint64_t initvalue);
    wasm_function& create_function(const char* name, wasm_type return_type, arguments_type arguments);

    void append_data_section(wasm_data_section& data);

    void export_as(const char* export_name, exportable& object);

    struct global_sym{
        std::string name;
        access_type access;
        wasm_type typ;
        uint64_t initvalue;
    };
    
    std::vector<wasm_memory> memories;
    std::vector<wasm_data_section> data_sections;
    std::vector<global_sym> globals;
    std::vector<std::reference_wrapper<exportable>> exports;
    std::map<std::string, wasm_function> functions;
};