#include <sstream>
#include <algorithm>
#include "StringTokenStream.h"

namespace Popper {

    Popper::StringTokenStream::StringTokenStream(std::string str, bool make_lower, char delimiter) {

        if(make_lower) {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        }
        std::stringstream ss(str);

        std::string section;
        while (getline(ss, section, delimiter)) {
            token_holder.push_back(section);
        }
    }

    std::string Popper::StringTokenStream::NextToken() {
        std::string ret = token_holder.front();
        token_holder.pop_front();
        return ret;
    }

    bool Popper::StringTokenStream::HasNext() {
        return !token_holder.empty();
    }
}
