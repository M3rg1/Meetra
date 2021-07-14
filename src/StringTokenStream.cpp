#include <sstream>
#include "StringTokenStream.h"

namespace Meetra {

    StringTokenStream::StringTokenStream(std::string str, char delimiter) {

        std::stringstream ss(str);

        std::string section;
        while (getline(ss, section, delimiter)) {
            token_holder.push_back(section);
        }
    }

    void StringTokenStream::MakeLower() {
        for (auto &item : token_holder) {
            std::transform(item.begin(), item.end(), item.begin(), ::tolower);
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
