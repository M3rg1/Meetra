#ifndef MEETRA_STRINGTOKENSTREAM_H
#define MEETRA_STRINGTOKENSTREAM_H

#include <deque>
#include <string>

namespace Meetra {

    class StringTokenStream {

    public:
        explicit StringTokenStream(const std::string& str, char delimiter = ' ');
        std::string NextToken();
        void MakeLower();
        bool HasNext();

    private:
        std::deque<std::string> token_holder;

    };

}


#endif //MEETRA_STRINGTOKENSTREAM_H
