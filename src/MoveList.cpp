#include "MoveList.h"
#include <iostream>


namespace Meetra {

    // TODO template color + type of move list
    // and then all the mnethods called by this mvoe list can be color templated as well
    MoveList::MoveList(const Board &board, MoveListType t) : board(board) {
        genPhase = BEST_MOVE;
    }

    Move MoveList::GetNextMove() {
        while (moves.empty()) {
            if (board.ColorToMove() == WHITE) {
                GenNewMoves<WHITE>();
            } else {
                GenNewMoves<BLACK>();
            }
        }

        // https://www.chessprogramming.org/Move_Ordering -- "Typical move ordering"
        // selection sort to pick the best move - pass through the whole list once and pick move with highest score

        Move m = moves.front();
        moves.pop_front();
        return m;
    }

    // TODO the whole class will be template, for either normal move list or quietsearch move list
    // TODO this function will be templated so that the switch is only

    template<Color C>
    inline void MoveList::GenNewMoves() {
        switch (genPhase) {
            case BEST_MOVE:
                // return TT / killer move // or make case: Killer Move (also from history heuristic possible)
                // also null move? PV? etc.
                break;
            case EVASION:
                GenMoves<EVASION, C>(board, moves);
                break;
            case CAPTURE:
                GenMoves<CAPTURE, C>(board, moves);
                break;
            case QUIET:
                GenMoves<QUIET, C>(board, moves);
                break;
            default:
                moves.emplace_back(INVALID_MOVE);
                break;
        }
        ++genPhase;
    }

}
