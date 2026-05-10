#include "args_parse.hpp"
#include <functional>
#include <iterator>
#include <vector>

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
    std::vector<std::string> options;
    std::string value_name;
    std::string description;
    std::function<bool(ArgvIterator&, ArgvIterator const&)> parser;
};

static ArgumentDescriptor make_flag_descriptor(
    std::vector<std::string> options,
    std::string description,
    bool& target
)
{
    return ArgumentDescriptor{
        options,
        "",
        description,
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
    std::string value_name,
    std::string description,
    std::string& target
)
{
    return ArgumentDescriptor{
        options,
        value_name,
        description,
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
        make_value_descriptor({"-o", "--output"}, "PATH", "Path to the output file", args.output_path),
        make_value_descriptor({"-i", "--input"}, "PATH", "Path to the input file", args.input_path),
        make_flag_descriptor({"--dump-ast"}, "Dump the AST and exit", args.dump_ast),
        make_flag_descriptor({"--dump-anf"}, "Dump the ANF and exit", args.dump_anf),
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
    std::string help_text = "Options:\n";
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
