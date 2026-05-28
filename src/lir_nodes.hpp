DEF_NODE(lir, number, lir_base)
    DEF_SCALAR_FIELD(long long, number)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(lir, char_literal, lir_base)
    DEF_SCALAR_FIELD(uint32_t, ch)
DEF_END

DEF_NODE(lir, bool_literal, lir_base)
    DEF_SCALAR_FIELD(bool, value)
DEF_END

DEF_NODE(lir, string_literal, lir_base)
    // index into the module string literals table
    DEF_SCALAR_FIELD(size_t, table_index)
    DEF_SCALAR_FIELD(uint32_t, size)
DEF_END

DEF_NODE(lir, record_init, lir_base)
    DEF_SCALAR_WITH_CONTEXT(type_id, record_type)
    // field initialisation expressions in record definition field order
    DEF_NODE_LIST_FIELD(lir, lir_node, values)
DEF_END

DEF_NODE(lir, load_expression, lir_base)
    DEF_SCALAR_FIELD(lir_place, source)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(lir, store_expression, lir_base)
    DEF_SCALAR_FIELD(lir_place, target)
    DEF_NODE_FIELD(lir, lir_node, value)
    DEF_SCALAR_WITH_CONTEXT(type_id, stored_type)
DEF_END

DEF_NODE(lir, if_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, condition)
    DEF_NODE_LIST_FIELD(lir, lir_node, then_code)
    DEF_NODE_LIST_FIELD(lir, lir_node, else_code)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(lir, while_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, condition)
    DEF_NODE_LIST_FIELD(lir, lir_node, while_code)
DEF_END

DEF_NODE(lir, function_call, lir_base)
    DEF_SCALAR_FIELD(std::string, function_name)
    DEF_NO_DUMP(symbol_id, symbol_ref)
    DEF_NODE_LIST_FIELD(lir, lir_node, arguments)
DEF_END

DEF_NODE(lir, cast_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, expression)
    DEF_SCALAR_WITH_CONTEXT(type_id, cast_type)
    source_range op_range;
DEF_END

DEF_NODE(lir, discard_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, expression)
DEF_END

DEF_NODE(lir, return_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, expression)
    DEF_SCALAR_FIELD(bool, is_explicit)
DEF_END

DEF_NODE(lir, break_statement, lir_base)
DEF_END

DEF_NODE(lir, unary_expression, lir_base)
    DEF_NODE_FIELD(lir, lir_node, expr)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(lir, binary_expression, lir_base)
    DEF_SCALAR_FIELD(lir_binary_op, operation)
    DEF_NODE_FIELD(lir, lir_node, left)
    DEF_NODE_FIELD(lir, lir_node, right)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
DEF_END

DEF_NODE(lir, function_head, lir_located_node)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_WITH_CONTEXT(function_signature, signature)
DEF_END

DEF_NODE(lir, import_definition, lir_located_node)
    DEF_SCALAR_FIELD(std::string, ns_name)
    DEF_NODE_FIELD(lir, lir_function_head, function_head)
    DEF_NO_DUMP(std::optional<std::string>, alias)
DEF_END

DEF_NODE(lir, function_definition, lir_attributed_node)
    DEF_NODE_FIELD(lir, lir_function_head, function_head)
    DEF_NODE_LIST_FIELD(lir, lir_node, code)
    DEF_NO_DUMP(std::unique_ptr<scope>, function_scope)
DEF_END

DEF_NODE(lir, global_definition, lir_located_node)
    DEF_SCALAR_FIELD(std::string, name)
    DEF_SCALAR_WITH_CONTEXT(type_id, assigned_type)
    DEF_NO_DUMP(symbol_id, symbol_ref)
    DEF_SCALAR_FIELD(int32_t, init_value)
    // DEF_NODE_FIELD(lir, lir_node, init_expression)
DEF_END

DEF_NODE(lir, module, lir_located_node)
    DEF_NODE_LIST_FIELD(lir, lir_import_definition, imports)
    DEF_NODE_LIST_FIELD(lir, lir_global_definition, globals)
    DEF_NODE_LIST_FIELD(lir, lir_function_definition, functions)
DEF_END
