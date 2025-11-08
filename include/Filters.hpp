#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

// Base class
class Filter {
    public:
        explicit Filter(const std::string& n) : m_Filename(n) {}
        virtual ~Filter() = default;
    
    protected:
        std::string m_Filename;
};

// Derived class: filters out entire lines
class FilterLines : public Filter {
    public:
        FilterLines(const std::string& filename, const std::unordered_set<uint64_t> &set)
            : Filter(filename), m_Lines(set) {}

        // returns if or not the filter contains the number
        bool Contains(const uint64_t n);

    private:
        std::unordered_set<uint64_t> m_Lines;
};

// Derived class: filters out parts (segments) of a line
class FilterSegment : public Filter {
    public:
        FilterSegment(const std::string& filename, const std::string& start, const std::string &end, 
                    const bool removeAll, const uint64_t line)
            : Filter(filename), m_Start(start), m_End(end), m_removeAll(removeAll), m_Line(line) {}

        // Filters out the part of the string specified
        std::string Apply(const std::string &s);
        uint64_t Line() {return m_Line;}

    private:
        uint64_t m_Line;
        std::string m_Start;
        std::string m_End;
        const bool m_removeAll;
};
