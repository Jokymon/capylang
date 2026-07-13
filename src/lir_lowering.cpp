#include "lir_lowering.hpp"
#include "tools.hpp"

lir_lowering::lir_lowering(context& ctx)
: visitor()
, parse_context(ctx)
{
}

void lir_lowering::leave(lir::store_record_statement& expr)
{
    auto record_type_id = parse_context.record_behind(expr.stored_type);
    CAPY_ASSERT(record_type_id.has_value(), "LIR lowering expected a record or pointer-to-record type");

    const auto& type_spec = parse_context.types[to_index(record_type_id.value())];
    auto* record = get_type_from_node<record_type>(type_spec);
    CAPY_ASSERT(record != nullptr, "LIR lowering expected a concrete record type");

    lir::statement_list replacement;
    if (parse_context.is_pointer_type(expr.stored_type))
    {
        lir::allocate_record_expression alloc_expr{
            .type = record_type_id.value()
        };

        lir::store_statement rec_ptr_store{
            .target = expr.target,
            .value = std::make_unique<lir::expr>(lir::expr{std::move(alloc_expr)}),
            .stored_type = expr.stored_type
        };

        replacement.push_back(std::make_unique<lir::statement>(std::move(rec_ptr_store)));
    }

    for (auto& init : expr.initialisations)
    {
        CAPY_ASSERT(init.field_index < record->fields.size(), "LIR record field initialisation index out of bounds");

        lir::place field_target = expr.target;
        field_target.projection.push_back(lir::place_elem{lir::field{init.field_index}});

        lir::store_statement field_store{
            .target = std::move(field_target),
            .value = std::move(init.value),
            .stored_type = parse_context.resolved_type(record->fields[init.field_index].second)
        };
        replacement.push_back(std::make_unique<lir::statement>(std::move(field_store)));
    }

    replace_statement(std::move(replacement));
}
