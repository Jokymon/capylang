DEF_NODE(node, number, node_base)
    DEF_SCALAR_FIELD(long long, number)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(node, bool_literal, node_base)
    DEF_SCALAR_FIELD(bool, value)
DEF_END

DEF_NODE(node, char_literal, node_base)
    DEF_SCALAR_FIELD(uint32_t, ch)
DEF_END

DEF_NODE(node, string_literal, node_base)
    // index into the string literals table
    DEF_SCALAR_FIELD(size_t, table_index)
    DEF_SCALAR_FIELD(uint32_t, size)
DEF_END

DEF_NODE(node, var_reference, node_base)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_WITH_CONTEXT(symbol_id, symbol_ref)
    DEF_SCALAR_FIELD(assign_context, context)
DEF_END

DEF_NODE(node, discard_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, expression)
DEF_END

DEF_NODE(node, pointer_deref, node_base)
    DEF_NODE_FIELD(node, node_expr, pointer_expression)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(node, function_call, node_base)
    DEF_SCALAR_FIELD(std::string, function_name)
    DEF_SCALAR_WITH_CONTEXT(symbol_id, symbol_ref)
    DEF_NODE_LIST_FIELD(node, node_expr, parameter)
DEF_END

DEF_NODE(node, cast_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, expression)
    DEF_SCALAR_WITH_CONTEXT(type_id, cast_type)
    source_range op_range;
DEF_END

DEF_NODE(node, return_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, expression)
    // used to distinguish return expressions that come from explicit uses of
    // keyword 'return' or if it was implicitly created from the last expression
    // in a function
    DEF_SCALAR_FIELD(bool, is_explicit)
DEF_END

DEF_NODE(node, break_statement, node_base)
DEF_END

DEF_NODE(node, unary_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, expr)
    DEF_SCALAR_FIELD(operator_type, operation)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(node, binary_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, left)
    DEF_NODE_FIELD(node, node_expr, right)
    source_range op_range;
    DEF_SCALAR_FIELD(operator_type, operation)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(node, let_expression, node_base)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_WITH_CONTEXT(symbol_id, symbol_ref)
    DEF_NODE_FIELD(node, node_expr, init_expression)
DEF_END

DEF_NODE(node, if_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, condition)
    DEF_NODE_LIST_FIELD(node, node_expr, then_code)
    DEF_NODE_LIST_FIELD(node, node_expr, else_code)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
    // TODO: should be introduce a new scope for the if?
DEF_END

DEF_NODE(node, while_expression, node_base)
    DEF_NODE_FIELD(node, node_expr, condition)
    DEF_NODE_LIST_FIELD(node, node_expr, while_code)
DEF_END

DEF_NODE(node, record_definition, node_base)
    DEF_SCALAR_FIELD(std::string, name)
    // TODO: dumping of record field definitions
    std::vector<record_type::field_type> fields;
DEF_END

DEF_NODE(node, field_initialisation, node_base)
    source_position location;
    DEF_SCALAR_FIELD(std::string, field_name)
    DEF_NODE_FIELD(node, node_expr, init_expression)
DEF_END

DEF_NODE(node, record_initialisation, node_base)
    DEF_SCALAR_WITH_CONTEXT(type_id, type_spec)
    DEF_LIST_FIELD(node, node_field_initialisation, initialisations)
DEF_END

DEF_NODE(node, field_deref, node_base)
    DEF_NODE_FIELD(node, node_expr, object)
    DEF_SCALAR_FIELD(std::string, fieldname)
    DEF_SCALAR_WITH_CONTEXT(type_id, object_type)

    // give a textual representation of the object for this dereferencing AST node
    // without including the field name
    DEF_NO_DUMP_PLAIN(std::string repr_obj() const;)
DEF_END

DEF_NODE(node, function_head, located_node)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_FIELD(function_signature, signature)
DEF_END

DEF_NODE(node, import_definition, located_node)
    DEF_SCALAR_FIELD(std::string, ns_name)
    DEF_NODE_FIELD(node, node_function_head, function_head)

    DEF_NO_DUMP(std::optional<std::string>, alias)
DEF_END

DEF_NODE(node, global_definition, located_node)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
    DEF_SCALAR_WITH_CONTEXT(symbol_id, symbol_ref)
    DEF_SCALAR_FIELD(int32_t, init_value)
    // std::unique_ptr<node_expr> init_expression;
DEF_END

DEF_NODE(node, function_definition, located_node_with_attributes)
    DEF_NODE_FIELD(node, node_function_head, function_head)
    DEF_NODE_LIST_FIELD(node, node_expr, code)

    DEF_NO_DUMP(std::unique_ptr<scope>, function_scope)
DEF_END

DEF_NODE(node, module, located_node)
    DEF_NODE_LIST_FIELD(node, node_import_definition, imports)
    DEF_NODE_LIST_FIELD(node, node_global_definition, globals)
    DEF_NODE_LIST_FIELD(node, node_function_definition, functions)
    DEF_NODE_LIST_FIELD(node, node_expr, typedefs)

    DEF_NO_DUMP(std::unique_ptr<scope>, module_scope)
DEF_END
