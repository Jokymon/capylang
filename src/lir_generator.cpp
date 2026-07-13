#include "lir_generator.hpp"
#include "semantics.hpp"
#include "tools.hpp"

lir::binary_op_t from_op_type(operator_type op)
{
    switch (op)
    {
        case op_multiply:
            return lir::binary_op_t::Multiply;
        case op_division:
            return lir::binary_op_t::Division;
        case op_modulus:
            return lir::binary_op_t::Modulus;
        case op_plus:
            return lir::binary_op_t::Plus;
        case op_minus:
            return lir::binary_op_t::Minus;
        case op_equals:
            return lir::binary_op_t::Equals;
        case op_notequals:
            return lir::binary_op_t::NotEquals;
        case op_lessthan:
            return lir::binary_op_t::LessThan;
        case op_lessthan_equal:
            return lir::binary_op_t::LessThanEqual;
        case op_greaterthan:
            return lir::binary_op_t::GreaterThan;
        case op_greaterthan_equal:
            return lir::binary_op_t::GreaterThanEqual;
        case op_and:
            return lir::binary_op_t::AndOp;
        case op_or:
            return lir::binary_op_t::OrOp;
        case op_shl:
            return lir::binary_op_t::Shl;
        case op_shr:
            return lir::binary_op_t::Shr;

        case op_assignment:
        case op_conversion:
            CAPY_FAIL("This operator type shouldn't be converted to LIR; enumval=%u", uint32_t(op));
    }
}

lir::place get_place(const node_expr& expr);

lir::place get_place(const node_var_reference& node)
{
    return lir::place{
        .base = {
            .name = node.name,
            .symbol_ref = node.symbol_ref
        },
        .projection = {}
    };
}

lir::place get_place(const node_let_expression& node)
{
    return lir::place{
        .base = {
            .name = node.name,
            .symbol_ref = node.symbol_ref
        },
        .projection = {}
    };
}

lir::place get_place(const node_pointer_deref& node)
{
    auto place = get_place(*node.pointer_expression);
    place.projection.push_back(lir::place_elem{lir::deref{}});

    return place;
}

lir::place get_place(const node_expr& expr)
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
                return lir::place{};
            }
        },
        expr.value
    );
}

lir_generator::lir_generator(context& ctx)
: parse_context(ctx)
{
}

lir::module lir_generator::generate(const node_module& module_def)
{
    lir::module mod;

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

lir::expr_list lir_generator::generate_exprs(const node_number& node)
{
    lir::number stmt{
        .value = node.number,
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_char_literal& node)
{
    lir::char_literal stmt{
        .ch = node.ch,
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_bool_literal& node)
{
    lir::bool_literal stmt{
        .value = node.value,
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_string_literal& node)
{
    lir::string_literal stmt{
        .table_index = node.table_index,
        .size = node.size
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_var_reference& node)
{
    // We assume, we only ever get here from an RHS context and we really
    // only have to generate load expressions. The LHS case should already
    // be handled by let-expressions and the assignment operator.
    auto& sym = parse_context.symbol_at(node.symbol_ref);
    lir::load_expression stmt{
        .source = get_place(node),
        .assigned_type = parse_context.resolved_type(sym.symbol_type)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_pointer_deref& node)
{
    lir::expr_list result = generate_to_exprs(*node.pointer_expression);
    CAPY_ASSERT(result.size() >= 1, "Pointer expression of pointer deref should contain some nodes");

    if (auto* load_expr = std::get_if<lir::load_expression>(result.back().get()))
    {
        // We already have a load expression, so we only need to add the pointer
        // dereferencing to the projections
        load_expr->source.projection.push_back(lir::place_elem{lir::deref{}});
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

lir::expr_list lir_generator::generate_exprs(const node_if_expression& node)
{
    auto condition = generate_to_exprs(*node.condition);

    lir::statement_list then_code;
    for (const auto& expression : node.then_code)
    {
        auto generated_code = generate_to_stmts(*expression);
        for (auto& stmt : generated_code)
        {
            then_code.push_back(std::move(stmt));
        }
    }

    lir::statement_list else_code;
    for (const auto& expression : node.else_code)
    {
        auto generated_code = generate_to_stmts(*expression);
        for (auto& stmt : generated_code)
        {
            else_code.push_back(std::move(stmt));
        }
    }

    lir::if_expression expr{
        .condition = std::move(condition[0]),
        .then_code = std::move(then_code),
        .else_code = std::move(else_code),
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(expr)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_record_definition& node)
{
    // In LIR there is no representation for the definition of a record type,
    // instead the record type has been added to the symbol table as new type
    return {};
}

lir::expr_list lir_generator::generate_exprs(const node_record_initialisation& node)
{
    CAPY_FAIL("LIR generator doesn't expect record initialisations outside of direct assignments");
    lir::expr_list result;
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_field_deref& node)
{
    lir::expr_list result = generate_to_exprs(*node.object);
    CAPY_ASSERT(result.size() == 1, "Field dereference expression should contain exactly one node");

    if (auto* load_expr = std::get_if<lir::load_expression>(result.back().get()))
    {
        // We already have a load expression, so we need to find the field index and add a
        // corresponding lir_field to the projection field
        const auto& type_spec = parse_context.types[to_index(node.object_type)];
        auto* rec_type = get_type_from_node<record_type>(type_spec);
        // If we still see a pointer here, then something in normalization went wrong
        CAPY_ASSERT(rec_type != nullptr, "LIR generator expects field access to already be normalized");

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

        load_expr->source.projection.push_back(lir::place_elem{lir::field{field_index}});
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

lir::expr_list lir_generator::generate_exprs(const node_function_call& node)
{
    lir::expr_list arguments;
    for (const auto& expression : node.parameter)
    {
        auto generated_arg = generate_to_exprs(*expression);
        CAPY_ASSERT(generated_arg.size() == 1, "Function call operands should only generate one LIR node");
        arguments.push_back(std::move(generated_arg[0]));
    }

    lir::function_call expr{
        .function_name = node.function_name,
        .symbol_ref = node.symbol_ref,
        .arguments = std::move(arguments)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(expr)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_cast_expression& node)
{
    lir::expr_list expressions = generate_to_exprs(*node.expression);
    CAPY_ASSERT(expressions.size() == 1, "LIR generation for cast expression should produce exactly one expression");
    lir::cast_expression stmt{
        .expression = std::move(expressions[0]),
        .cast_type = parse_context.resolved_type(node.cast_type)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_to_exprs(const node_expr& expression)
{
    return std::visit(
        [&](auto&& arg)
        {
            if constexpr (requires { this->generate_exprs(arg); })
            {
                return this->generate_exprs(arg);
            }
            else
            {
                CAPY_FAIL("Specialized node expression type is not expected, index: %lu", expression.value.index());
                return lir::expr_list{};
            }
        },
        expression.value
    );
}

lir::statement_list lir_generator::generate_to_stmts(const node_expr& expression)
{
    lir::statement_list result;

    expression.value.visit(
        [&](auto&& expr) -> void
        {
            if constexpr (requires { this->generate_stmts(expr); })
            {
                result = generate_stmts(expr);
            }
            else if constexpr (requires { this->generate_exprs(expr); })
            {
                auto exprs = generate_exprs(expr);
                for (auto& expr : exprs)
                {
                    lir::expression_statement stmt{
                        .expression = std::move(expr),
                        .is_dropped = false
                    };
                    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
            }
        }
    );

    // In case an expression hasn't generated anything, maybe something was
    // instead stored in the assignments member. So let's fetch those now
    for (auto& assignment_stmt : assignments)
    {
        result.push_back(std::move(assignment_stmt));
    }
    assignments.clear();

    return result;
}

lir::statement_list lir_generator::generate_stmts(const node_return_expression& node)
{
    lir::expr_list expressions = generate_to_exprs(*node.expression);
    lir::return_statement stmt{
        .expression = std::move(expressions[0]),
        .is_explicit = node.is_explicit
    };
    lir::statement_list result;
    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
    return result;
}

lir::statement_list lir_generator::generate_stmts(const node_break_statement& node)
{
    lir::break_statement stmt;
    lir::statement_list result;
    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
    return result;
}

lir::statement_list lir_generator::generate_stmts(const node_discard_expression& node)
{
    lir::statement_list result;

    node.expression->value.visit(
        [&](auto&& expr) -> void
        {
            if constexpr (requires { this->generate_stmts(expr); })
            {
                result = generate_stmts(expr);
            }
            else if constexpr (requires { this->generate_exprs(expr); })
            {
                auto exprs = generate_exprs(expr);
                for (auto& expr : exprs)
                {
                    lir::expression_statement stmt{
                        .expression = std::move(expr),
                        .is_dropped = true
                    };
                    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
            }
        }
    );

    return result;
}

lir::statement_list lir_generator::generate_stmts(const node_let_expression& node)
{
    auto target = get_place(node);
    lir::statement_list result;

    if (node.init_expression)
    {
        node.init_expression->value.visit(
            [&](const auto& inite) -> void
            {
                using INIT_TYPE = std::decay_t<decltype(inite)>;

                if constexpr (std::is_same_v<INIT_TYPE, node_record_initialisation>)
                {
                    std::vector<lir::field_initialisation> initialisations;
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
                                auto field_init_expr = generate_to_exprs(*init.init_expression);
                                initialisations.push_back(
                                    lir::field_initialisation{field_index, std::move(field_init_expr[0])}
                                );
                                break;
                            }
                            field_index++;
                        }
                    }

                    lir::store_record_statement stmt{
                        .target = target,
                        .initialisations = std::move(initialisations),
                        .stored_type = inite.type_spec
                    };
                    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
                else if constexpr (std::is_same_v<INIT_TYPE, node_string_literal>)
                {
                    lir::store_string_statement stmt{
                        .target = target,
                        .string_table_index = inite.table_index,
                        .string_size = inite.size
                    };

                    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
                else
                {
                    lir::expr_list init_expressions;
                    if (node.init_expression)
                    {
                        init_expressions = generate_to_exprs(*node.init_expression);
                    }

                    std::unique_ptr<lir::expr> init_expr = nullptr;
                    CAPY_ASSERT(init_expressions.size() <= 1, "There should be maximum of one init expression");
                    if (init_expressions.size() >= 1)
                    {
                        init_expr = std::move(init_expressions[0]);
                    }

                    auto& sym = parse_context.symbol_at(node.symbol_ref);
                    lir::store_statement stmt{
                        .target = target,
                        .value = std::move(init_expr),
                        .stored_type = parse_context.resolved_type(sym.symbol_type)
                    };

                    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
            }
        );
    }

    return result;
}

lir::statement_list lir_generator::generate_stmts(const node_while_expression& node)
{
    auto condition = generate_to_exprs(*node.condition);

    lir::statement_list while_body;
    for (const auto& expression : node.while_code)
    {
        auto generated_code = generate_to_stmts(*expression);
        for (auto& expr : generated_code)
        {
            while_body.push_back(std::move(expr));
        }
    }

    lir::while_statement stmt{
        .condition = std::move(condition[0]),
        .while_code = std::move(while_body)
    };
    lir::statement_list result;
    result.push_back(std::make_unique<lir::statement>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_unary_expression& node)
{
    lir::expr_list expressions = generate_to_exprs(*node.expr);
    lir::unary_expression stmt{
        .expr = std::move(expressions[0]),
        .assigned_type = parse_context.resolved_type(node.assigned_type)
    };
    lir::expr_list result;
    result.push_back(std::make_unique<lir::expr>(std::move(stmt)));
    return result;
}

lir::expr_list lir_generator::generate_exprs(const node_binary_expression& node)
{
    auto right = generate_to_exprs(*node.right);
    CAPY_ASSERT(right.size() == 1, "Generated LIR for right binary operand should have returned exactly one node");

    lir::expr_list result;
    if (node.operation != op_assignment)
    {
        auto left = generate_to_exprs(*node.left);
        CAPY_ASSERT(left.size() == 1, "Generated LIR for left binary operand should have returned exactly one node");
        lir::binary_expression expr{
            .op = from_op_type(node.operation),
            .left = std::move(left[0]),
            .right = std::move(right[0]),
            .assigned_type = parse_context.resolved_type(node.assigned_type),
        };
        result.push_back(std::make_unique<lir::expr>(std::move(expr)));
    }
    else
    {
        lir::place target = get_place(*node.left);

        node.right->value.visit(
            [&](const auto& rhs) -> void
            {
                using RHS_TYPE = std::decay_t<decltype(rhs)>;

                if constexpr (std::is_same_v<RHS_TYPE, node_record_initialisation>)
                {
                    auto record_type_id = parse_context.record_behind(rhs.type_spec);
                    CAPY_ASSERT(record_type_id.has_value(), "LIR: the record initialisation should be on a record type or pointer to record type");

                    const auto& type_spec = parse_context.types[to_index(record_type_id.value())];
                    auto* rec_type = get_type_from_node<record_type>(type_spec);

                    std::vector<lir::field_initialisation> initialisations;
                    for (const auto& init : rhs.initialisations)
                    {
                        size_t field_index = 0;
                        for (const auto& field_def : rec_type->fields)
                        {
                            if (init.field_name == field_def.first)
                            {
                                auto field_init_expr = generate_to_exprs(*init.init_expression);
                                initialisations.push_back(
                                    lir::field_initialisation{field_index, std::move(field_init_expr[0])}
                                );
                                break;
                            }
                            field_index++;
                        }
                    }

                    lir::store_record_statement stmt{
                        .target = target,
                        .initialisations = std::move(initialisations),
                        .stored_type = rhs.type_spec
                    };
                    assignments.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
                // TODO: can we just implement store_string the same as in the let-statements? But then maybe we
                // could factor this whole block out into some more generic function, so as not to repeat this all.
                else
                {
                    lir::store_statement stmt{
                        .target = target,
                        .value = std::move(right[0])
                    };
                    assignments.push_back(std::make_unique<lir::statement>(std::move(stmt)));
                }
            }
        );
    };
    return result;
}

std::unique_ptr<lir::import_definition> lir_generator::generate(const node_import_definition& imp_def)
{
    auto imp = std::make_unique<lir::import_definition>();
    imp->function_head.name = imp_def.function_head->name;
    imp->function_head.signature = imp_def.function_head->signature;

    imp->ns_name = imp_def.ns_name;
    imp->alias = imp_def.alias;

    return imp;
}

std::unique_ptr<lir::global_definition> lir_generator::generate(const node_global_definition& global_def)
{
    auto glob = std::make_unique<lir::global_definition>();
    glob->name = global_def.name;
    glob->assigned_type = global_def.assigned_type;
    glob->symbol_ref = global_def.symbol_ref;
    glob->init_value = global_def.init_value;

    return glob;
}

std::unique_ptr<lir::function_definition> lir_generator::generate(const node_function_definition& func_def)
{
    auto func = std::make_unique<lir::function_definition>();
    for (const auto& attr : func_def.attributes)
    {
        func->attributes.push_back(attr.name);
    }
    func->head.name = func_def.function_head->name;
    func->head.signature = func_def.function_head->signature;

    for (const auto& statement : func_def.code)
    {
        auto generated_code = generate_to_stmts(*statement);
        for (auto& stmt : generated_code)
        {
            func->code.push_back(std::move(stmt));
        }
    }

    func->function_scope = std::make_unique<scope>(*func_def.function_scope);
    return func;
}
