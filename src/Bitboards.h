#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Types.h"
#include <bit>

namespace Meetra::Bitboards {

    void Init();

    [[nodiscard]] Bitboard GetUnboundRookMoves(Square s);
    [[nodiscard]] Bitboard GetUnboundBishopMoves(Square s);
    [[nodiscard]] Bitboard GetRayBetweenEdges(Square s1, Square s2);
    [[nodiscard]] Bitboard GetRayBetweenSquares(Square s1, Square s2);

    template<PieceType PT>
    [[nodiscard]] Bitboard GetAttacksForPiece(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE);
    [[nodiscard]] std::string PPBitboard(Bitboard b);
    [[nodiscard]] inline bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] inline int PopCount(Bitboard b) { return std::__popcount(b); }
    [[nodiscard]] inline Square Lsb(Bitboard b) { return static_cast<Square>(__builtin_ctzll(b)); }
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
