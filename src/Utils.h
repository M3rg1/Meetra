#ifndef MEETRA_UTILS_H
#define MEETRA_UTILS_H


namespace Meetra::Utils {

    inline bool IsPositiveNumber(const std::string &str) {
        return str.find_first_not_of("0123456789") == std::string::npos;
    }

    inline bool AllUniqueChars(std::string str) {
        std::sort(str.begin(), str.end());
        for (int i = 0; i < str.length() - 1; i++) {
            if (str[i] == str[i + 1]) {
                return false;
            }
        }
        return true;
    }

    inline bool ContainsOnlyChars(const std::string &str, const std::string &chars) {
        return str.find_first_not_of(chars) == std::string::npos;
    }

}




#endif //MEETRA_UTILS_H
