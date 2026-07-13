#pragma once
// Code for lowering the high level LIR node types to slighly
// lower level nodes. The lowering aspects currently involve
//
//  * Resolve record initialisations into individual record
//    assignments

#include "lir.hpp"
#include "lir_visitor.hpp"
#include "parser.hpp"

class lir_lowering : public lir::visitor
{
public:
    explicit lir_lowering(context& ctx);
    void lower(lir::module& module);

protected:
    void leave(lir::store_record_statement& expr) override;

private:
    context& parse_context;
};