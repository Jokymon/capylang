#include <string>
#include <iterator>

class ArgvIterator {
public:
    using value_type = std::string;
    using reference = std::string;
    using pointer = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    ArgvIterator(char** ptr) : current(ptr) {}

    reference operator*() const { return std::string(*current); }

    ArgvIterator& operator++() { ++current; return *this; }
    ArgvIterator operator++(int) { ArgvIterator tmp = *this; ++current; return tmp; }

    bool operator==(const ArgvIterator& other) const { return current == other.current; }
    bool operator!=(const ArgvIterator& other) const { return current != other.current; }

private:
    char** current;
};

class ArgvRange {
public:
    ArgvRange(int argc, char* argv[]) : begin_(argv), end_(argv + argc) {}

    ArgvIterator begin() const { return ArgvIterator(begin_); }
    ArgvIterator end() const { return ArgvIterator(end_); }

private:
    char** begin_;
    char** end_;
};
