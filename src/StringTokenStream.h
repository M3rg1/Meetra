#ifndef MEETRA_STRINGTOKENSTREAM_H
#define MEETRA_STRINGTOKENSTREAM_H

#include <deque>
#include <string>

namespace Meetra {

    /**
     * Splits a string delimited by whitespace into tokens and converts them to lowercase.
     */
    class StringTokenStream {

    public:
        explicit StringTokenStream(std::string str, char delimiter = ' ');
        std::string NextToken();
        void MakeLower();
        bool HasNext();

    private:
        std::deque<std::string> token_holder;

    };

}


#endif //MEETRA_STRINGTOKENSTREAM_H
