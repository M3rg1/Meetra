#ifndef MEETRA_MOVELIST_H
#define MEETRA_MOVELIST_H

#include "Types.h"
#include "MoveGenerator.h"
#include <deque>
#include "Board.h"

namespace Meetra {

    // TODO priority queue for move list?


    class MoveList {

    public:
        MoveList(const Board &board, MoveListType t = NORMAL);
        Move GetNextMove();


    private:
        Color color;
        GenPhase genPhase;
        std::deque<Move> moves;
        const Board &board;

        inline void GenNewMoves();
    };

}

#endif //MEETRA_MOVELIST_H
