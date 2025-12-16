#pragma once
#include <map>
#include <ostream>
#include <string>
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

class wasm_statement
{
public:
    virtual ~wasm_statement();

    virtual void dump(std::ostream &output, size_t indent) const =0;
};

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

struct wasm_instruction : public wasm_statement
{
    explicit wasm_instruction(wasm_op op);

    wasm_op op;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_index : public wasm_instruction
{
    explicit wasm_op_index(wasm_op op, const std::string var_name, uint32_t index);

    std::string name;
    uint32_t index;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_type : public wasm_instruction
{
    explicit wasm_op_type(wasm_op op, wasm_type type);

    wasm_type value_type;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_type_sign : public wasm_instruction
{
    explicit wasm_op_type_sign(wasm_op op, wasm_type type);

    wasm_type value_type;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_type_value : public wasm_instruction
{
    explicit wasm_op_type_value(wasm_op op, wasm_type type, uint64_t value);

    wasm_type value_type;
    uint64_t value;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_align_offset : public wasm_instruction
{
    explicit wasm_op_align_offset(wasm_op op, wasm_type type, uint32_t alignment, uint64_t offset);

    wasm_type value_type;
    uint32_t alignment;
    uint64_t offset;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_label : public wasm_instruction
{
    explicit wasm_op_label(wasm_op op, wasm_branch_label label);

    wasm_branch_label label;

    void dump(std::ostream &output, size_t indent) const override;
};

struct wasm_op_func : public wasm_instruction
{
    explicit wasm_op_func(wasm_op op, wasm_function_ref function);

    wasm_function_ref function;

    void dump(std::ostream &output, size_t indent) const override;
};

class wasm_block;
class wasm_if_block : public wasm_statement
{
public:
    explicit wasm_if_block(wasm_type return_type);
    std::pair<wasm_block&, wasm_block&> blocks();

    void dump(std::ostream &output, size_t indent) const override;

private:
    wasm_type return_type;
    std::unique_ptr<wasm_block> then_block;
    std::unique_ptr<wasm_block> else_block;
};

class wasm_block : public wasm_statement
{
public:
    wasm_block();
    void dump(std::ostream &output, size_t indent) const override;

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
    void eqz(wasm_type type);
    void br(wasm_branch_label branch_label);
    void br_if(wasm_branch_label branch_label);
    void call(const char* function_name);

protected:
    wasm_branch_label block_label;

private:
    std::vector<std::unique_ptr<wasm_statement>> instructions;
};

class wasm_internal_block : public wasm_block
{
public:
    explicit wasm_internal_block(wasm_type return_type);
    void dump(std::ostream &output, size_t indent) const override;

private:
    wasm_type return_type;
};

class wasm_loop_block : public wasm_block
{
public:
    explicit wasm_loop_block(wasm_type return_type);
    void dump(std::ostream &output, size_t indent) const override;

private:
    wasm_type return_type;
};

class exportable
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

    void dump_export(std::ostream &output, size_t indent) const;

protected:
    std::string name;
    index_type index;

private:
    std::string export_name;
    std::string export_type;
};

class wasm_function : public exportable
{
public:
    void dump(std::ostream &output, size_t indent) const;

    bool is_imported() const;

    void allocate_local(const char* name, wasm_type var_type);
    wasm_block& body();
    void import_from(const char* ns, const char* import_name);

private:
    friend class wasm_module;
    explicit wasm_function(const char* name, wasm_type return_type, arguments_type arguments);

private:
    wasm_type return_type;
    arguments_type arguments;
    arguments_type locals;
    wasm_block function_body;
    std::string ns;
    std::string import_name;
};

class wasm_data_section
{
public:
    explicit wasm_data_section(size_t offset);

    void dump(std::ostream &output, size_t indent) const;

    size_t allocate_data(const std::string& data);

private:
    size_t init_offset;
    size_t offset;
    std::string data_buffer;
};

class wasm_memory : public exportable
{
public:
    explicit wasm_memory(index_type index, size_t initial_block_count);

    void dump(std::ostream &output, size_t indent) const;

private:
    size_t initial_block_count;
};

class wasm_module
{
public:
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

    void dump(std::ostream &output) const;

private:
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