#ifndef MEETRA_MOVEGENERATOR_H
#define MEETRA_MOVEGENERATOR_H

#include "MoveList.h"
#include "Bitboards.h"
#include "Types.h"
#include "Board.h"
#include <deque>

namespace Meetra {

    template<GenPhase phase, Color c>
    void GenMoves(const Board &board, std::deque<Move> &d);

}


#endif //MEETRA_MOVEGENERATOR_H
