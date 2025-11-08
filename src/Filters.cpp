#include <Filters.hpp>

bool FilterLines::Contains(const uint64_t n)
{
    return m_Lines.contains(n);
}

std::string FilterSegment::Apply(const std::string &s)
{
    std::string result;
    std::size_t pos = 0;

    while (true) {
        std::size_t startPos = s.find(m_Start, pos);
        if (startPos == std::string::npos) {
            result.append(s.substr(pos));
            break;
        }

        std::size_t endPos = s.find(m_End, startPos + m_Start.size());
        if (endPos == std::string::npos) {
            result.append(s.substr(pos));
            break;
        }

        result.append(s.substr(pos, startPos + m_Start.size() - pos));
        pos = endPos;

        if (!m_removeAll) {
            result.append(s.substr(pos));
            break;
        }
    }

    return result;
}

