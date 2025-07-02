#pragma once
#include "parser.hpp"
#include <optional>

std::optional<node_parse_error> semantic_analysis(ast_node &root);
