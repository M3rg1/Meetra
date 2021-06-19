#ifndef POPPER_STRINGTOKENSTREAM_H
#define POPPER_STRINGTOKENSTREAM_H

#include <string>
#include <deque>

namespace Popper {

    /**
     * Splits a string delimited by whitespace into tokens and converts them to lowercase.
     */
    class StringTokenStream {

    public:
        explicit StringTokenStream(std::string str, bool make_lower = false, char delimiter = ' ');
        std::string NextToken();
        bool HasNext();

    private:
        std::deque<std::string> token_holder;

    };

}


#endif //POPPER_STRINGTOKENSTREAM_H
