#pragma once
#include "parser.hpp"
#include <map>
#include <optional>
#include <string>

struct function_def {
    enum source {
        internal,   // local defined, private or exported
        external    // importet function
    };

    source_range location;
};

struct variable_def {
    type_kind type_spec;
};

struct scope {
    std::map<std::string, function_def> functions;
    scope* parent;
};

std::optional<node_parse_error> semantic_analysis(ast_node &root);
