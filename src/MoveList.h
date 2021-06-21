#ifndef MEETRA_MOVELIST_H
#define MEETRA_MOVELIST_H

#include "Types.h"

namespace Meetra {

    class MoveList {

    public:
        inline constexpr Move GetNextMove();
        inline constexpr void AddCapture(Move m);
        inline constexpr void AddQuiet(Move m);
        inline constexpr void AddMove(Move m);


    private:
        // first 128 for captures, next 128 for quiets
        Move moves[256];
        int move_count = 0;
        //int captures_count = 0;
        //int quiets_count = 128;
    };

}

#endif //MEETRA_MOVELIST_H
