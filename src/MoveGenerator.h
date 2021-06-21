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
        MoveList * GenerateMoves(MoveList * move_list, PieceType piece_type, Board board);
        void DoStuff();

    private:

    };

}


#endif //MEETRA_MOVEGENERATOR_H
