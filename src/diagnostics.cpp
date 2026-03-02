#include "diagnostics.hpp"
#include <utility>

namespace
{
const char* to_string(diagnostic_severity severity)
{
    switch (severity)
    {
        case diagnostic_severity::error:
            return "error";
        case diagnostic_severity::warning:
            return "warning";
        case diagnostic_severity::note:
            return "note";
    }

    return "error";
}
}

void diagnostic_bag::add(diagnostic entry)
{
    diagnostics_.push_back(std::move(entry));
}

void diagnostic_bag::add(
    source_position location,
    const std::string& message,
    diagnostic_severity severity,
    diagnostic_phase phase
)
{
    diagnostics_.push_back(diagnostic{
        .location = std::move(location),
        .severity = severity,
        .phase = phase,
        .message = message,
    });
}

bool diagnostic_bag::empty() const
{
    return diagnostics_.empty();
}

bool diagnostic_bag::has_errors() const
{
    for (const auto& entry : diagnostics_)
    {
        if (entry.severity == diagnostic_severity::error)
        {
            return true;
        }
    }
    return false;
}

void diagnostic_bag::clear()
{
    diagnostics_.clear();
}

const std::vector<diagnostic>& diagnostic_bag::items() const
{
    return diagnostics_;
}

diagnostic_emitter::diagnostic_emitter(diagnostic_phase phase)
: phase_(phase)
{
}

void diagnostic_emitter::emit_diagnostic(
    source_position location,
    const std::string& message,
    diagnostic_severity severity
)
{
    diagnostics_sink().add(std::move(location), message, severity, phase_);
}

void diagnostic_emitter::append_error_at(source_position location, const std::string& message)
{
    emit_diagnostic(std::move(location), message, diagnostic_severity::error);
}

void print_diagnostics(std::ostream& out, const diagnostic_bag& diagnostics)
{
    for (const auto& entry : diagnostics.items())
    {
        out << entry.location.filename << ":" << entry.location.line << ":" << entry.location.column << ": ";
        if (entry.severity != diagnostic_severity::error)
        {
            out << to_string(entry.severity) << ": ";
        }
        out << entry.message << "\n";
    }
}
