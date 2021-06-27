#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Types.h"
#include <deque>
#include "Board.h"

namespace Meetra {

    // TODO priority queue for move list?


    // TODO TODO FIXME

    // RENAME THIS MOVE GENERATOR AND MOVE MOVEGENERATOR CODE HERE
    // DOESNT MAKE SENSE TO HAVE IT ANY OTHER WAY
    // WE JUST CREATE NEW MOVE GENERATOR FOR EVERY ITERATION
    // AND KEEP ASKING IT FOR NEXT MOVE
    // THIS SHIT WITH MOVE LIST IS WEIRD AF

    // TODO TODO TODO FIXME

    class MoveGen {

    public:
        MoveGen(const Board &board, GenPhase start_phase = BEST_MOVE);
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

#endif //MEETRA_MOVEGEN_H
