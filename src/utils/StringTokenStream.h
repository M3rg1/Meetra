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
        explicit StringTokenStream(std::string str);
        std::string nextToken();
        bool hasNext();

    private:
        std::deque<std::string> tokenHolder;

    };

}


#endif //POPPER_STRINGTOKENSTREAM_H
