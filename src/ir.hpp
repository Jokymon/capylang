#pragma once
#include <map>
#include <ostream>
#include <string>
#include <vector>

enum class wasm_type
{
    none,
    u8,
    i8,
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

class wasm_simple_statement : public wasm_statement
{
public:
    explicit wasm_simple_statement(const std::string& instr);

    void dump(std::ostream &output, size_t indent) const override;

private:
    std::string instr;
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
    std::string label() const;

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
    void br(const char* branch_label);
    void br_if(const char* branch_label);
    void call(const char* function_name);

protected:
    std::string block_label;

private:
    std::vector<std::unique_ptr<wasm_statement>> instructions;

private:
    static int label_index;
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