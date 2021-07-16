#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Types.h"
#include <bit>

namespace Meetra::Bitboards {

    void Init();

    Bitboard GetUnboundRookMoves(Square s);
    Bitboard GetUnboundBishopMoves(Square s);
    Bitboard GetRayBetweenEdges(Square s1, Square s2);
    Bitboard GetRayBetweenSquares(Square s1, Square s2);

    template<PieceType PT>
    Bitboard GetAttacksForPiece(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE);
    std::string PrettyPrint(Bitboard b);
    inline bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    inline int PopCount(Bitboard b) { return std::__popcount(b); }
    inline Square Lsb(Bitboard b) { return static_cast<Square>(__builtin_ctzll(b)); }
    inline Square PopLsb(Bitboard &b) {
        const Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    // win64 https://www.chessprogramming.org/BitScan -> Processor Instructions for Bitscans
    /*inline Square Lsb(Bitboard b) {
        unsigned long idx;
        _BitScanForward64(&idx, b);
        return (Square) idx;
    }*/

}

#endif //MEETRA_BITBOARDS_H
