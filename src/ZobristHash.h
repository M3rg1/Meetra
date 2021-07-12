#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H


#include "Types.h"

namespace Meetra{

    class Board;

    typedef uint64_t ZobristHash;
    typedef uint32_t Key32;

    void InitZobrist();
    ZobristHash GenZobristHash(Board &board);
    inline Key32 Make32Key(ZobristHash zobrist_hash) { return zobrist_hash >> 32; }


}


#endif //MEETRA_ZOBRISTHASH_H
