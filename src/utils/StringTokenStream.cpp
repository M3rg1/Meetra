#include <sstream>
#include <algorithm>
#include "StringTokenStream.h"

namespace Popper {

    Popper::StringTokenStream::StringTokenStream(std::string str) {

        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        std::stringstream ss(str);

        std::string section;
        while (getline(ss, section, ' ')) {
            tokenHolder.push_back(section);
        }
    }

    std::string Popper::StringTokenStream::nextToken() {
        std::string ret = tokenHolder.front();
        tokenHolder.pop_front();
        return ret;
    }

    bool Popper::StringTokenStream::hasNext() {
        return !tokenHolder.empty();
    }
}
