#include "MoveGenerator.h"
#include "Bitboards.h"

namespace Meetra {


    void GenCaptures(const Board &board, std::deque<Move> &d) {

    }

    void GenQuiets(const Board &board, std::deque<Move> &d) {

    }

    void GenPromotions(const Board &board, std::deque<Move> &d) {

    }

    template<GenPhase phase>
    void GenMoves(const Board &board, std::deque<Move> &d){

        if(PopCount(board.GetCheckers()) > 1 && phase != EVASION){
            return;
        }

        switch (phase){
            case EVASION:
                // TODO GENERATE ONLY KING MOVES - KING MOVE TO NON-DANGER SQUARES
                if(PopCount(board.GetCheckers())) {
                    // checkers exist - calc king moves
                    return;
                }
                break;
            case PROMOTION:
                GenPromotions(board, d);
                break;
            case CAPTURE:
                GenCaptures(board, d);
                break;
            case QUIET:
                GenQuiets(board, d);
                break;
        }
    }

    template void GenMoves<EVASION>(const Board &board, std::deque<Move> &d);
    template void GenMoves<CAPTURE>(const Board &board, std::deque<Move> &d);
    template void GenMoves<PROMOTION>(const Board &board, std::deque<Move> &d);
    template void GenMoves<QUIET>(const Board &board, std::deque<Move> &d);

}