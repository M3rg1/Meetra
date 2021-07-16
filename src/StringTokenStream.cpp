#include <sstream>
#include "StringTokenStream.h"

namespace Meetra {

    StringTokenStream::StringTokenStream(const std::string& str, char delimiter) {
        std::stringstream ss(str);
        std::string section;
        while (getline(ss, section, delimiter)) {
            token_holder.push_back(section);
        }
    }

    void StringTokenStream::MakeLower() {
        for (auto &str : token_holder) {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        }
    }

    std::string StringTokenStream::NextToken() {
        std::string ret = token_holder.front();
        token_holder.pop_front();
        return ret;
    }

    bool StringTokenStream::HasNext() {
        return !token_holder.empty();
    }
}
