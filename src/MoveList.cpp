#include "MoveList.h"
#include <iostream>


namespace Meetra {

    MoveList::MoveList(const Board &board, MoveListType t) : board(board) {
        genPhase = BEST_MOVE;
        color = board.ColorToMove();
    }

    Move MoveList::GetNextMove() {
        while (moves.empty()) {
            GenNewMoves();
            // sort moves
        }

        Move m = moves.front();
        moves.pop_front();
        return m;
    }

    // TODO the whole class will be template, for either normal move list or quietsearch move list
    // TODO this function will be templated so that the switch is only

    inline void MoveList::GenNewMoves() {
        switch (genPhase) {
            case BEST_MOVE:
                // return TT / killer move // or make case: Killer Move (also from history heuristic possible)
                // also null move? PV? etc.
                break;
            case EVASION:
                GenMoves<EVASION>(board, moves, color);
                break;
            case PROMOTION:
                GenMoves<PROMOTION>(board, moves, color);
                break;
            case CAPTURE:
                GenMoves<CAPTURE>(board, moves, color);
                break;
            case QUIET:
                GenMoves<QUIET>(board, moves, color);
                break;
            default:
                moves.push_back(INVALID_MOVE);
                break;
        }
        ++genPhase;
    }

}
