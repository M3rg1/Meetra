#ifndef POPPER_BITBOARDS_H
#define POPPER_BITBOARDS_H

#include "Types.h"
#include <bit>
#include <string>

namespace Popper {

    std::string PPStringBitboard(Bitboard b);

    // linux builtins
    inline constexpr Square Lsb(Bitboard b) {
        return Square(__builtin_ctzll(b));
    }

    inline constexpr Square Msb(Bitboard b) {
        return Square(63 ^ __builtin_clzll(b));
    }

    inline constexpr Square PopLsb(Bitboard &b) {
        const Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    inline constexpr int PopCount(Bitboard b) {
        return std::__popcount(b);
    }


    //squareIndex = 8*rankIndex + fileIndex
    //rankIndex   = squareIndex div 8
    //fileIndex   = squareIndex mod 8



    // inline Bitboard SquareToBitboard(Square s){} ..
}

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



#endif //POPPER_BITBOARDS_H
