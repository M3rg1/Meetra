#ifndef MEETRA_MOVEGENERATOR_H
#define MEETRA_MOVEGENERATOR_H

#include "MoveList.h"
#include "Types.h"
#include "Board.h"

namespace Meetra {

    extern ulong RookMagic[SQUARE_NR];
    extern ulong BishopMagic[SQUARE_NR];


    class MoveGenerator {

    public:
        template<Color Us, PieceType Pt>
        MoveList * GenerateMoves(PieceType piece_type, const Board& board);

    private:

    };

}


#endif //MEETRA_MOVEGENERATOR_H
