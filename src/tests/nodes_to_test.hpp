DEF_NODE(tst, single_scalar, root)
    DEF_SCALAR_FIELD(std::string, name)
DEF_END

DEF_NODE(tst, no_dump_fields, root)
    DEF_SCALAR_FIELD(std::string, name1)
    DEF_NO_DUMP(int, value)
    DEF_SCALAR_FIELD(std::string, name2)
DEF_END

DEF_NODE(tst, single_nesting, root)
    DEF_NODE_FIELD(tst, tst_single_scalar, child)
DEF_END

DEF_NODE(tst, struct_with_vector, root)
    DEF_LIST_FIELD(tst, tst_single_scalar, child)
DEF_END

DEF_NODE(tst, struct_with_struct_list, root)
    DEF_NODE_LIST_FIELD(tst, tst_single_scalar, child)
DEF_END

DEF_NODE(tst, struct_with_context_value, root)
    DEF_SCALAR_WITH_CONTEXT(custom_type, child)
DEF_END