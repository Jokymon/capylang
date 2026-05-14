#include "args_parse.hpp"
#include "tools.hpp"
#include <functional>
#include <iterator>
#include <vector>

// maximum length to use when listing the arguments in the help text
constexpr int MAX_HELP_LENGTH = 90;

class ArgvIterator
{
public:
    using value_type = std::string;
    using reference = std::string;
    using pointer = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    ArgvIterator(char** ptr)
    : current(ptr)
    {
    }

    reference operator*() const { return std::string(*current); }

    ArgvIterator& operator++()
    {
        ++current;
        return *this;
    }
    ArgvIterator operator++(int)
    {
        ArgvIterator tmp = *this;
        ++current;
        return tmp;
    }

    bool operator==(const ArgvIterator& other) const { return current == other.current; }
    bool operator!=(const ArgvIterator& other) const { return current != other.current; }

private:
    char** current;
};

class ArgvRange
{
public:
    ArgvRange(int argc, char* argv[])
    : begin_(argv)
    , end_(argv + argc)
    {
    }

    ArgvIterator begin() const { return ArgvIterator(begin_); }
    ArgvIterator end() const { return ArgvIterator(end_); }

private:
    char** begin_;
    char** end_;
};

struct ArgumentDescriptor
{
    enum arg_type_t
    {
        flag,
        option
    };

    arg_type_t arg_type;
    std::vector<std::string> options;
    std::string value_name;
    std::string description;
    bool required;
    std::function<bool(ArgvIterator&, ArgvIterator const&)> parser;
};

static ArgumentDescriptor make_flag_descriptor(
    std::vector<std::string> options,
    std::string description,
    bool& target
)
{
    return ArgumentDescriptor{
        ArgumentDescriptor::flag,
        options,
        "",
        description,
        false, // flags should never be required arguments
        [&target, options = options](ArgvIterator& current, ArgvIterator const&)
        {
            std::string arg = *current;
            for (auto const& option : options)
            {
                if (arg == option)
                {
                    target = true;
                    return true;
                }
            }
            return false;
        }
    };
}

static ArgumentDescriptor make_value_descriptor(
    std::vector<std::string> options,
    bool required,
    std::string value_name,
    std::string description,
    std::string& target
)
{
    return ArgumentDescriptor{
        ArgumentDescriptor::option,
        options,
        value_name,
        description,
        required,
        [&target, options = options](ArgvIterator& current, ArgvIterator const& end)
        {
            std::string arg = *current;
            for (auto const& option : options)
            {
                if (arg == option)
                {
                    if (++current == end)
                    {
                        return false;
                    }
                    target = *current;
                    return true;
                }
                const std::string prefix = option + "=";
                if (arg.rfind(prefix, 0) == 0)
                {
                    target = arg.substr(prefix.size());
                    return true;
                }
            }
            return false;
        }
    };
}

static std::vector<ArgumentDescriptor> make_argument_descriptors(Args& args)
{
    return {
        make_value_descriptor({"-o", "--output"}, true, "PATH", "Path to the output file. Depending on the suffix, a WAT or WASM file is generated", args.output_path),
        make_value_descriptor({"-i", "--input"}, true, "PATH", "Path to the capylang source input file", args.input_path),
        make_flag_descriptor({"--dump-ast"}, "Only dump the AST and exit", args.dump_ast),
        make_flag_descriptor({"--dump-anf"}, "Only dump the ANF and exit", args.dump_anf),
        make_flag_descriptor({"--help"}, "Show this help message and exit", args.help),
    };
}

static std::string describe_argument(ArgumentDescriptor const& descriptor)
{
    std::string text;
    bool first = true;
    for (auto const& option : descriptor.options)
    {
        if (!first)
        {
            text += ", ";
        }
        first = false;
        text += option;
    }
    if (!descriptor.value_name.empty())
    {
        text += " ";
        text += descriptor.value_name;
    }
    return text;
}

std::string generate_help_text(Args const& args)
{
    std::vector<ArgumentDescriptor> descriptors = make_argument_descriptors(const_cast<Args&>(args));
    std::string help_text = "Overview: Capylang compiler\n\n";

    std::string options_line = "Usage: " + args.programm_name + " ";
    std::string indentation = std::string(" ", options_line.size());
    for (auto const& descriptor : descriptors)
    {
        std::string options_text = "";
        if (!descriptor.required)
        {
            options_text += "[";
        }

        options_text += join(descriptor.options.begin(), descriptor.options.end(), "|");

        if (descriptor.arg_type == ArgumentDescriptor::option)
        {
            options_text += "=" + descriptor.value_name;
        }
        if (!descriptor.required)
        {
            options_text += "]";
        }

        if (options_text.size() + 1 /* for the space in front of the additional option */ + options_line.size() <= MAX_HELP_LENGTH)
        {
            // This option still fits on the current line
            options_line += " " + options_text;
        }
        else
        {
            // This option no longer fits on the current line, so break the line and put the
            // option on the next line
            help_text += options_line;
            options_line = indentation + options_text;
        }
    }

    help_text += options_line;

    help_text += "\n\n";
    help_text += "Options:\n";
    for (auto const& descriptor : descriptors)
    {
        help_text += "  ";
        help_text += describe_argument(descriptor);
        help_text += "\n      ";
        help_text += descriptor.description;
        help_text += "\n";
    }
    return help_text;
}

Args parse_args(int argc, char* argv[])
{
    Args arguments;

    arguments.programm_name = std::string(argv[0]);

    std::vector<ArgumentDescriptor> descriptors = make_argument_descriptors(arguments);
    ArgvRange arg_range{argc, argv};
    auto arg = arg_range.begin();
    auto args_end = arg_range.end();

    for (; arg != args_end; ++arg)
    {
        for (auto& descriptor : descriptors)
        {
            ArgvIterator temp = arg;
            if (descriptor.parser(temp, args_end))
            {
                arg = temp;
                break;
            }
        }
    }
    return arguments;
}
