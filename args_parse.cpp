#include "args_parse.hpp"
#include <iterator>

class ArgvIterator
{
public:
    using value_type = std::string;
    using reference = std::string;
    using pointer = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    ArgvIterator(char **ptr) : current(ptr) {}

    reference operator*() const { return std::string(*current); }

    ArgvIterator &operator++()
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

    bool operator==(const ArgvIterator &other) const { return current == other.current; }
    bool operator!=(const ArgvIterator &other) const { return current != other.current; }

private:
    char **current;
};

class ArgvRange
{
public:
    ArgvRange(int argc, char *argv[]) : begin_(argv), end_(argv + argc) {}

    ArgvIterator begin() const { return ArgvIterator(begin_); }
    ArgvIterator end() const { return ArgvIterator(end_); }

private:
    char **begin_;
    char **end_;
};

void parse_option(const char *option, ArgvIterator &start, const ArgvIterator &end, std::string &output_string)
{
    if ((*start).starts_with(option))
    {
        if ((*start).size() == strlen(option))
        {
            // we have to take the argument for the given option from the next
            // argument
            if (++start != end)
            {
                output_string = *start;
            }
        }
        else
        {
            // The option is already included in the argument and has to be
            // removed
            output_string = (*start).substr(strlen(option));
        }
    }
}

Args parse_args(int argc, char *argv[])
{
    Args arguments;

    ArgvRange args{argc, argv};
    auto args_start = args.begin();
    auto args_end = args.end();

    for (; args_start != args_end; args_start++)
    {
        parse_option("-o", args_start, args_end, arguments.output_path);
        parse_option("-i", args_start, args_end, arguments.input_path);
    }
    return arguments;
}