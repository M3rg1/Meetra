#ifndef MEETRA_UTILS_H
#define MEETRA_UTILS_H

#include <algorithm>

namespace Meetra::Utils {

    inline bool AllUniqueChars(std::string str) {
        std::ranges::sort(str);
        for (size_t i = 0; i < str.length() - 1; i++) {
            if (str[i] == str[i + 1]) {
                return false;
            }
        }
        return true;
    }

    inline bool ContainsOnlyChars(std::string_view str, std::string_view chars) {
        return str.find_first_not_of(chars) == std::string_view::npos;
    }

    inline bool IsPositiveNumber(std::string_view str) {
        return !str.empty() && ContainsOnlyChars(str, "0123456789");
    }

}




#endif //MEETRA_UTILS_H
