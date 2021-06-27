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
        GenPhase genPhase;
        std::deque<Move> moves;
        const Board &board;
        Bitboard checkers;
        Bitboard legal_moves;

        template <Color C>
        inline void GenNewMoves();
        inline Bitboard SquareAttackers(Square s, Color attacked_by, Bitboard occ) const;
    };

}

#endif //MEETRA_MOVELIST_H
