#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Defs.h"
#include <bit>

namespace Meetra::Bitboards {

    void Init();

    template<PieceType PT>
    [[nodiscard]] Bitboard GetAttacks(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE);
    [[nodiscard]] Bitboard GetRookRays(Square s);
    [[nodiscard]] Bitboard GetBishopRays(Square s);
    [[nodiscard]] Bitboard GetRayToBorders(Square s1, Square s2);
    [[nodiscard]] Bitboard GetRayToSquares(Square s1, Square s2);
    [[nodiscard]] Bitboard RankMask(Rank r);

    [[nodiscard]] inline bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] inline bool ExactlyOne(Bitboard b) { return b && !MoreThanOne(b); }
    [[nodiscard]] inline int PopCount(Bitboard b) { return std::__popcount(b); }
    [[nodiscard]] inline Square Msb(Bitboard b) { return 63 ^ __builtin_clzll(b); }
    [[nodiscard]] inline Square Lsb(Bitboard b) { return __builtin_ctzll(b); }
    inline Square PopLsb(Bitboard &b) {
        Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    [[nodiscard]] std::string PPBitboard(Bitboard b);

    template<Direction D>
    constexpr Bitboard Shift(Bitboard b) {
        if constexpr (D == NORTH) return b << 8;
        else if constexpr (D == SOUTH) return b >> 8;
        else if constexpr (D == EAST) return (b & ~0x8080808080808080UL) << 1;
        else if constexpr (D == WEST) return (b & ~0x0101010101010101UL) >> 1;
        else if constexpr (D == NORTH_EAST) return (b & ~0x8080808080808080UL) << 9;
        else if constexpr (D == NORTH_WEST) return (b & ~0x0101010101010101UL) << 7;
        else if constexpr (D == SOUTH_EAST) return (b & ~0x8080808080808080UL) >> 7;
        else if constexpr (D == SOUTH_WEST) return (b & ~0x0101010101010101UL) >> 9;
        else return EMPTY_BB;
    }
}

#endif //MEETRA_BITBOARDS_H
