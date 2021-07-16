#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H


#include "Types.h"


namespace Meetra {

    class Board;

    namespace Zobrist {

        void Init();
        ZobristHash GenHash(Board &board);
        inline Key32 Make32Key(ZobristHash zobrist_hash) { return zobrist_hash >> 32; }

    }
}


#endif //MEETRA_ZOBRISTHASH_H
