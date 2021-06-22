#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Types.h"
#include <bit>
#include <string>
#include <iostream>

namespace Meetra {

    void InitBitboards();

    inline Bitboard GetRookAttacks(Square s, Bitboard occ);
    inline Bitboard GetBishopAttacks(Square s, Bitboard occ);

    std::string PPBitboard(Bitboard b);

    constexpr int PopCount(Bitboard b) { return std::__popcount(b); }
    //linux builtins
    constexpr Square Lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }
    constexpr Square Msb(Bitboard b) { return Square(63 ^ __builtin_clzll(b)); }

    constexpr Square PopLsb(Bitboard &b) {
        const Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    constexpr Bitboard SquareToBB(Square s) { return 1UL << s; }
    constexpr void SetBBSquareOne(Bitboard &b, Square s) { b |= SquareToBB(s); }
    constexpr void SetBBSquareZero(Bitboard &b, Square s) { b &= ~SquareToBB(s); }

// win64 https://www.chessprogramming.org/BitScan -> Processor Instructions for Bitscans
/*inline Square Lsb(Bitboard b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square) idx;
}

inline Square Msb(Bitboard b) {
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square) idx;
}*/

}

#endif //MEETRA_BITBOARDS_H
