#ifndef POPPER_MOVEGENERATOR_H
#define POPPER_MOVEGENERATOR_H

#include "MoveList.h"
#include "Types.h"
#include "Board.h"

namespace Popper {

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


#endif //POPPER_MOVEGENERATOR_H
