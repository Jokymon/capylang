#pragma once
#include "locations.hpp"
#include <ostream>
#include <string>
#include <vector>

enum class diagnostic_severity
{
    error,
    warning,
    note
};

enum class diagnostic_phase
{
    lexer,
    parser,
    type_inference,
    semantics,
    anf,
    emitter
};

struct diagnostic
{
    source_position location;
    diagnostic_severity severity = diagnostic_severity::error;
    diagnostic_phase phase = diagnostic_phase::parser;
    std::string message;
};

class diagnostic_bag
{
public:
    void add(diagnostic entry);
    void add(
        source_position location,
        const std::string& message,
        diagnostic_severity severity,
        diagnostic_phase phase
    );

    bool empty() const;
    bool has_errors() const;
    void clear();

    const std::vector<diagnostic>& items() const;

private:
    std::vector<diagnostic> diagnostics_;
};

class diagnostic_emitter
{
protected:
    explicit diagnostic_emitter(diagnostic_bag& diagnostics_sink, diagnostic_phase phase);
    virtual ~diagnostic_emitter() = default;

    void emit_diagnostic(
        source_position location,
        const std::string& message,
        diagnostic_severity severity = diagnostic_severity::error
    );
    void append_error_at(source_position location, const std::string& message);

private:
    diagnostic_bag& diagnostics_sink_;
    diagnostic_phase phase_;
};

void print_diagnostics(std::ostream& out, const diagnostic_bag& diagnostics);
