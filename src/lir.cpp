#include "lir.hpp"
#include "tools.hpp"
#include <iostream>

inline std::ostream& operator<<(std::ostream& o, lir_binary_op op)
{
    switch (op)
    {
        case lir_binary_op::multiply:
            o << "*";
            break;
        case lir_binary_op::division:
            o << "/";
            break;
        case lir_binary_op::modulus:
            o << "\"%\"";
            break;
        case lir_binary_op::plus:
            o << "+";
            break;
        case lir_binary_op::minus:
            o << "\"-\"";
            break;
        case lir_binary_op::equals:
            o << "\"==\"";
            break;
        case lir_binary_op::notequals:
            o << "\"!=\"";
            break;
        case lir_binary_op::lessthan:
            o << "\"<\"";
            break;
        case lir_binary_op::lessthan_equal:
            o << "\"<=\"";
            break;
        case lir_binary_op::greaterthan:
            o << "\">\"";
            break;
        case lir_binary_op::greaterthan_equal:
            o << "\">=\"";
            break;
        case lir_binary_op::and_op:
            o << "and";
            break;
        case lir_binary_op::or_op:
            o << "or";
            break;
        case lir_binary_op::shl:
            o << "\"<<\"";
            break;
        case lir_binary_op::shr:
            o << "\">>\"";
            break;
    }
    return o;
}

lir_binary_op from_op_type(operator_type op)
{
    switch (op)
    {
        case op_multiply:
            return lir_binary_op::multiply;
        case op_division:
            return lir_binary_op::division;
        case op_modulus:
            return lir_binary_op::modulus;
        case op_plus:
            return lir_binary_op::plus;
        case op_minus:
            return lir_binary_op::minus;
        case op_equals:
            return lir_binary_op::equals;
        case op_notequals:
            return lir_binary_op::notequals;
        case op_lessthan:
            return lir_binary_op::lessthan;
        case op_lessthan_equal:
            return lir_binary_op::lessthan_equal;
        case op_greaterthan:
            return lir_binary_op::greaterthan;
        case op_greaterthan_equal:
            return lir_binary_op::greaterthan_equal;
        case op_and:
            return lir_binary_op::and_op;
        case op_or:
            return lir_binary_op::or_op;
        case op_shl:
            return lir_binary_op::shl;
        case op_shr:
            return lir_binary_op::shr;

        case op_assignment:
        case op_conversion:
            CAPY_FAIL("This operator type shouldn't be converted to LIR; enumval=%u", uint32_t(op));
    }
}

inline std::ostream& operator<<(std::ostream& o, const lir_place_elem& elem)
{
    std::visit([&](auto&& e)
               {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, lir_deref>)
                    {
                        o << "*";
                    }
                    else if constexpr (std::is_same_v<T, lir_field>)
                    {
                        o << "_" << e.index;
                    } },
               elem);
    return o;
}

inline std::ostream& operator<<(std::ostream& o, const lir_place& value)
{
    o << "{ lir_place: { variable: " << value.base.name << ", projection: ";
    for (const auto& proj : value.projection)
    {
        o << proj << ", ";
    }
    o << "} }";
    return o;
}

// lir_val_repr() functions are used in DUMPER_CTX_DUMP_FUNC to dispatch on
// the context-based type to dump
std::string lir_val_repr(const context& ctx, type_id tid)
{
    return ctx.repr(tid);
}

std::string lir_val_repr(const context& ctx, const function_signature& fsig)
{
    return ctx.repr(fsig.function_type);
}

bool lir_attributed_node::has_attribute(const std::string& attr_name) const
{
    for (const auto& attr : attributes)
    {
        if (attr.name == attr_name)
        {
            return true;
        }
    }
    return false;
}

// --------------------------------------------------------

type_id lir_assigned_node_type(const lir_node& node, context& ctx)
{
    return std::visit(
        [&](const auto& n) -> type_id
        {
            using T = std::decay_t<decltype(n)>;

            if constexpr (std::is_same_v<T, lir_number>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_char_literal>)
            {
                return ctx.intern_primitive(primitive_type::Char);
            }
            else if constexpr (std::is_same_v<T, lir_bool_literal>)
            {
                return ctx.intern_primitive(primitive_type::Boolean);
            }
            else if constexpr (std::is_same_v<T, lir_string_literal>)
            {
                return ctx.intern_primitive(primitive_type::String);
            }
            else if constexpr (std::is_same_v<T, lir_store_record_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_load_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_store_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_if_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_while_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_function_call>)
            {
                const auto& sym = ctx.symbol_at(n.symbol_ref);
                CAPY_ASSERT(sym.kind == symbol_kind::function, "Compiler Error: LIR function call should resolve to function symbol");

                auto return_type = ctx.function_return_type(sym.signature.function_type);
                CAPY_ASSERT(return_type.has_value(), "Compiler Error: LIR function call type should have a valid return type");
                return return_type.value();
            }
            else if constexpr (std::is_same_v<T, lir_cast_expression>)
            {
                return n.cast_type;
            }
            else if constexpr (std::is_same_v<T, lir_discard_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_return_expression>)
            {
                return ctx.intern_primitive(primitive_type::Void);
            }
            else if constexpr (std::is_same_v<T, lir_unary_expression>)
            {
                return n.assigned_type;
            }
            else if constexpr (std::is_same_v<T, lir_binary_expression>)
            {
                return n.assigned_type;
            }
            else
            {
                CAPY_FAIL("Compiler error: LIR encountered unhandled type; index=%lu", node.value.index());
                return ILLEGAL_TYPE;
            }
        },
        node.value
    );
}

// --------------------------------------------------------

lir_place get_place(const node_expr& expr);

lir_place get_place(const node_var_reference& node)
{
    return lir_place{
        .base = {
            .name = node.name,
            .symbol_ref = node.symbol_ref
        },
        .projection = {}
    };
}

lir_place get_place(const node_let_expression& node)
{
    return lir_place{
        .base = {
            .name = node.name,
            .symbol_ref = node.symbol_ref
        },
        .projection = {}
    };
}

lir_place get_place(const node_pointer_deref& node)
{
    auto place = get_place(*node.pointer_expression);
    place.projection.push_back(lir_place_elem{lir_deref{}});

    return place;
}

lir_place get_place(const node_expr& expr)
{
    return std::visit(
        [&](auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, node_var_reference>)
            {
                return get_place(arg);
            }
            else if constexpr (std::is_same_v<T, node_pointer_deref>)
            {
                return get_place(arg);
            }
            else
            {
                CAPY_FAIL("LIR can't turn this node type into a place. index=%lu", expr.value.index());
                return lir_place{};
            }
        },
        expr.value
    );
}

// --------------------------------------------------------

lir_generator::lir_generator(context& ctx)
: parse_context(ctx)
{
}

lir_module lir_generator::generate(const node_module& module_def)
{
    lir_module mod;

    for (const auto& i : module_def.imports)
    {
        mod.imports.push_back(generate(*i));
    }
    for (const auto& g : module_def.globals)
    {
        mod.globals.push_back(generate(*g));
    }
    for (const auto& f : module_def.functions)
    {
        mod.functions.push_back(generate(*f));
    }

    return mod;
}

lir_node_list lir_generator::generate(const node_number& node)
{
    lir_number stmt{
        .number = node.number,
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_char_literal& node)
{
    lir_char_literal stmt{
        .ch = node.ch,
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_bool_literal& node)
{
    lir_bool_literal stmt{
        .value = node.value,
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_string_literal& node)
{
    lir_string_literal stmt{
        .table_index = node.table_index,
        .size = node.size
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_var_reference& node)
{
    // We assume, we only ever get here from an RHS context and we really
    // only have to generate load expressions. The LHS case should already
    // be handled by let-expressions and the assignment operator.
    auto& sym = parse_context.symbol_at(node.symbol_ref);
    lir_load_expression stmt{
        .source = get_place(node),
        .assigned_type = parse_context.resolved_type(sym.symbol_type)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_pointer_deref& node)
{
    lir_node_list result = generate(*node.pointer_expression);
    CAPY_ASSERT(result.size() >= 1, "Pointer expression of pointer deref should contain some nodes");

    if (auto* load_expr = std::get_if<lir_load_expression>(&result.back()->value))
    {
        // We already have a load expression, so we only need to add the pointer
        // dereferencing to the projections
        load_expr->source.projection.push_back(lir_place_elem{lir_deref{}});
        load_expr->assigned_type = parse_context.resolved_type(node.assigned_type);
    }
    else
    {
        // Here we assume that pointer dereferencing only happens on something that really is
        // dereferencable, but which in most cases probably is ultimately a variable of some kind.
        // If it really is a variable, then the pointer_expression has already returned a load
        // expression and we end up in the then-part above. If its not a variable, then this is
        // unexpected here.
        CAPY_FAIL("pointer dereferencing should only happen on existing load expressions");
    }

    return result;
}

lir_node_list lir_generator::generate(const node_let_expression& node)
{
    auto target = get_place(node);
    lir_node_list result;

    if (node.init_expression)
    {
        node.init_expression->value.visit(
            [&](const auto& inite) -> void
            {
                using INIT_TYPE = std::decay_t<decltype(inite)>;

                if constexpr (std::is_same_v<INIT_TYPE, node_record_initialisation>)
                {
                    std::vector<lir_field_initialisation> initialisations;
                    auto record_type_id = parse_context.record_behind(inite.type_spec);
                    CAPY_ASSERT(record_type_id.has_value(), "LIR: the record initialisation should be on a record type or pointer to record type");

                    const auto& type_spec = parse_context.types[to_index(record_type_id.value())];
                    auto* rec_type = get_type_from_node<record_type>(type_spec);

                    for (const auto& init : inite.initialisations)
                    {
                        size_t field_index = 0;
                        for (const auto& field_def : rec_type->fields)
                        {
                            if (init.field_name == field_def.first)
                            {
                                auto field_init_expr = generate(*init.init_expression);
                                initialisations.push_back(
                                    lir_field_initialisation{{}, field_index, std::move(field_init_expr[0])}
                                );
                                break;
                            }
                            field_index++;
                        }
                    }

                    lir_store_record_expression expr{
                        .target = target,
                        .initialisations = std::move(initialisations),
                        .stored_type = record_type_id.value()
                    };
                    result.push_back(std::make_unique<lir_node>(std::move(expr)));
                }
                else
                {
                    lir_node_list init_expressions;
                    if (node.init_expression)
                    {
                        init_expressions = generate(*node.init_expression);
                    }

                    std::unique_ptr<lir_node> init_expr = nullptr;
                    CAPY_ASSERT(init_expressions.size() <= 1, "There should be maximum of one init expression");
                    if (init_expressions.size() >= 1)
                    {
                        init_expr = std::move(init_expressions[0]);
                    }

                    auto& sym = parse_context.symbol_at(node.symbol_ref);
                    lir_store_expression stmt{
                        .target = target,
                        .value = std::move(init_expr),
                        .stored_type = parse_context.resolved_type(sym.symbol_type)
                    };

                    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
                }
            }
        );
    }

    return result;
}

lir_node_list lir_generator::generate(const node_if_expression& node)
{
    auto condition = generate(*node.condition);

    lir_node_list then_code;
    for (const auto& expression : node.then_code)
    {
        auto generated_code = generate(*expression);
        for (auto& expr : generated_code)
        {
            then_code.push_back(std::move(expr));
        }
    }

    lir_node_list else_code;
    for (const auto& expression : node.else_code)
    {
        auto generated_code = generate(*expression);
        for (auto& expr : generated_code)
        {
            else_code.push_back(std::move(expr));
        }
    }

    lir_if_expression expr{
        .condition = std::move(condition[0]),
        .then_code = std::move(then_code),
        .else_code = std::move(else_code),
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(expr)));
    return result;
}

lir_node_list lir_generator::generate(const node_while_expression& node)
{
    auto condition = generate(*node.condition);

    lir_node_list while_body;
    for (const auto& expression : node.while_code)
    {
        auto generated_code = generate(*expression);
        for (auto& expr : generated_code)
        {
            while_body.push_back(std::move(expr));
        }
    }

    lir_while_expression expr{
        .condition = std::move(condition[0]),
        .while_code = std::move(while_body)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(expr)));
    return result;
}

lir_node_list lir_generator::generate(const node_record_definition& node)
{
    // In LIR there is no representation for the definition of a record type,
    // instead the record type has been added to the symbol table as new type
    return {};
}

lir_node_list lir_generator::generate(const node_record_initialisation& node)
{
    CAPY_FAIL("LIR generator doesn't expect record initialisations outside of direct assignments");
    lir_node_list result;
    return result;
}

lir_node_list lir_generator::generate(const node_field_deref& node)
{
    lir_node_list result = generate(*node.object);
    CAPY_ASSERT(result.size() == 1, "Field dereference expression should contain exactly one node");

    if (auto* load_expr = std::get_if<lir_load_expression>(&result.back()->value))
    {
        // We already have a load expression, so we need to find the field index and add a
        // corresponding lir_field to the projection field
        const auto& type_spec = parse_context.types[to_index(node.object_type)];
        auto* rec_type = get_type_from_node<record_type>(type_spec);
        size_t field_index = 0;
        type_id field_type = ILLEGAL_TYPE;
        for (const auto& field : rec_type->fields)
        {
            if (field.first == node.fieldname)
            {
                field_type = parse_context.resolved_type(field.second);
                break;
            }
            field_index++;
        }

        load_expr->source.projection.push_back(lir_place_elem{lir_field{field_index}});
        load_expr->assigned_type = field_type;
    }
    else
    {
        // Here we assume that field dereferencing only happens on something that really is
        // dereferencable, but which in most cases probably is ultimately a variable of some kind.
        // If it really is a variable, then the object has already returned a load
        // expression and we end up in the then-part above. If its not a variable, then this is
        // unexpected here.
        CAPY_FAIL("Field dereferencing should only happen on existing load expressions");
    }

    return result;
}

lir_node_list lir_generator::generate(const node_function_call& node)
{
    lir_node_list arguments;
    for (const auto& expression : node.parameter)
    {
        auto generated_code = generate(*expression);
        CAPY_ASSERT(generated_code.size() == 1, "Function call operands should only generate one LIR node");
        arguments.push_back(std::move(generated_code[0]));
    }

    lir_function_call expr{
        .function_name = node.function_name,
        .symbol_ref = node.symbol_ref,
        .arguments = std::move(arguments)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(expr)));
    return result;
}

lir_node_list lir_generator::generate(const node_cast_expression& node)
{
    lir_node_list expressions = generate(*node.expression);
    CAPY_ASSERT(expressions.size() == 1, "LIR generation for cast expression should produce exactly one expression");
    lir_cast_expression stmt{
        .expression = std::move(expressions[0]),
        .cast_type = parse_context.resolved_type(node.cast_type)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_discard_expression& node)
{
    lir_node_list expressions = generate(*node.expression);
    CAPY_ASSERT(expressions.size() == 1, "LIR generation for cast expression should produce exactly one expression");
    lir_discard_expression stmt{
        .expression = std::move(expressions[0])
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_return_expression& node)
{
    lir_node_list expressions = generate(*node.expression);
    lir_return_expression stmt{
        .expression = std::move(expressions[0]),
        .is_explicit = node.is_explicit
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_unary_expression& node)
{
    lir_node_list expressions = generate(node);
    lir_unary_expression stmt{
        .expr = std::move(expressions[0]),
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_binary_expression& node)
{
    auto right = generate(*node.right);
    CAPY_ASSERT(right.size() == 1, "Generated LIR for right binary operand should have returned exactly one node");

    lir_node_list result;
    if (node.operation != op_assignment)
    {
        auto left = generate(*node.left);
        CAPY_ASSERT(left.size() == 1, "Generated LIR for left binary operand should have returned exactly one node");
        lir_binary_expression expr{
            .operation = from_op_type(node.operation),
            .left = std::move(left[0]),
            .right = std::move(right[0]),
            .assigned_type = parse_context.resolved_type(node.assigned_type),
        };
        result.push_back(std::make_unique<lir_node>(std::move(expr)));
    }
    else
    {
        lir_place target = get_place(*node.left);

        node.right->value.visit(
            [&](const auto& rhs) -> void
            {
                using RHS_TYPE = std::decay_t<decltype(rhs)>;

                if constexpr (std::is_same_v<RHS_TYPE, node_record_initialisation>)
                {
                    std::vector<lir_field_initialisation> initialisations;
                    auto record_type_id = parse_context.record_behind(rhs.type_spec);
                    CAPY_ASSERT(record_type_id.has_value(), "LIR: the record initialisation should be on a record type or pointer to record type");

                    const auto& type_spec = parse_context.types[to_index(record_type_id.value())];
                    auto* rec_type = get_type_from_node<record_type>(type_spec);

                    for (const auto& init : rhs.initialisations)
                    {
                        size_t field_index = 0;
                        for (const auto& field_def : rec_type->fields)
                        {
                            if (init.field_name == field_def.first)
                            {
                                auto field_init_expr = generate(*init.init_expression);
                                initialisations.push_back(
                                    lir_field_initialisation{{}, field_index, std::move(field_init_expr[0])}
                                );
                                break;
                            }
                            field_index++;
                        }
                    }

                    lir_store_record_expression expr{
                        .target = target,
                        .initialisations = std::move(initialisations)
                    };
                    result.push_back(std::make_unique<lir_node>(std::move(expr)));
                }
                else
                {
                    lir_store_expression expr{
                        .target = target,
                        .value = std::move(right[0])
                    };
                    result.push_back(std::make_unique<lir_node>(std::move(expr)));
                }
            }
        );
    };
    return result;
}

lir_node_list lir_generator::generate(const node_break_statement& node)
{
    lir_break_statement stmt;
    lir_node_list result;
    result.push_back(std::make_unique<lir_node>(std::move(stmt)));
    return result;
}

lir_node_list lir_generator::generate(const node_expr& expression)
{
    return std::visit([&](auto&& arg)
                      { return generate(arg); },
                      expression.value);
}

std::unique_ptr<lir_import_definition> lir_generator::generate(const node_import_definition& imp_def)
{
    auto imp = std::make_unique<lir_import_definition>();
    imp->function_head = std::make_unique<lir_function_head>();
    imp->function_head->name = imp_def.function_head->name;
    imp->function_head->signature = imp_def.function_head->signature;

    imp->ns_name = imp_def.ns_name;
    imp->alias = imp_def.alias;

    return imp;
}

std::unique_ptr<lir_global_definition> lir_generator::generate(const node_global_definition& global_def)
{
    auto glob = std::make_unique<lir_global_definition>();
    glob->name = global_def.name;
    glob->assigned_type = global_def.assigned_type;
    glob->symbol_ref = global_def.symbol_ref;
    glob->init_value = global_def.init_value;

    return glob;
}

std::unique_ptr<lir_function_definition> lir_generator::generate(const node_function_definition& func_def)
{
    auto func = std::make_unique<lir_function_definition>();
    for (const auto& attr : func_def.attributes)
    {
        func->attributes.push_back(lir_attribute{attr.name});
    }
    func->function_head = std::make_unique<lir_function_head>();
    func->function_head->name = func_def.function_head->name;
    func->function_head->signature = func_def.function_head->signature;

    for (const auto& expression : func_def.code)
    {
        auto generated_code = generate(*expression);
        for (auto& expr : generated_code)
        {
            func->code.push_back(std::move(expr));
        }
    }

    func->function_scope = std::make_unique<scope>(*func_def.function_scope);
    return func;
}

lir_visitor::lir_visitor()
{
}

void lir_visitor::visit(lir_node& root)
{
    root.value.visit(
        [&](auto& n) -> void
        {
            process(root.location, n);
        }
    );
}

void lir_visitor::visit_nodes(lir_module& module)
{
    for (const auto& import_def : module.imports)
    {
        process(import_def->location, *import_def);
    }
}

void lir_visitor::visit_nodes(lir_function_definition& func_def)
{
    for (const auto& expression : func_def.code)
    {
        visit(*expression);
    }
}

void lir_visitor::visit_nodes(lir_store_record_expression& sr_expr)
{
    for (const auto& init : sr_expr.initialisations)
    {
        visit(*init.value);
    }
}

void lir_visitor::visit_nodes(lir_store_expression& s_expr)
{
    visit(*s_expr.value);
}

void lir_visitor::visit_nodes(lir_if_expression& i_expr)
{
    visit(*i_expr.condition);

    for (const auto& expression : i_expr.then_code)
    {
        visit(*expression);
    }

    for (const auto& expression : i_expr.else_code)
    {
        visit(*expression);
    }
}

void lir_visitor::visit_nodes(lir_while_expression& w_expr)
{
    visit(*w_expr.condition);

    for (const auto& expression : w_expr.while_code)
    {
        visit(*expression);
    }
}

void lir_visitor::visit_nodes(lir_unary_expression& expr)
{
    visit(*expr.expr);
}

void lir_visitor::visit_nodes(lir_binary_expression& expr)
{
    visit(*expr.left);
    visit(*expr.right);
}

void lir_visitor::visit_nodes(lir_cast_expression& expr)
{
    visit(*expr.expression);
}

void lir_visitor::visit_nodes(lir_discard_expression& expr)
{
    visit(*expr.expression);
}

void lir_visitor::visit_nodes(lir_return_expression& expr)
{
    if (expr.expression)
    {
        visit(*expr.expression);
    }
}

void lir_visitor::visit_nodes(lir_function_call& func_call)
{
    for (const auto& param : func_call.arguments)
    {
        visit(*param);
    }
}

// --------------------------------------------------------

#define DEFINE_DUMP_FUNCS
#define DUMPER_CTX_TYPE const context&
#define DUMPER_CTX_DUMP_FUNC(cntxt, val) lir_val_repr(cntxt, val)
#include "dumpable.hpp"
#include "lir_nodes.hpp"
#undef DEFINE_DUMP_FUNCS

void dump_lir(std::ostream& os, DUMPER_CTX_TYPE ctx, const lir_node& node, int indent)
{
    std::visit([indent, ctx, &os](auto&& arg)
               { dump_lir(os, ctx, arg, indent); },
               node.value);
}