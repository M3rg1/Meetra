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
    [[nodiscard]] Bitboard GetAttacks(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE) ;
    [[nodiscard]] std::string PPBitboard(Bitboard b);
    [[nodiscard]] inline bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] inline bool ExactlyOne(Bitboard b) { return b && !MoreThanOne(b); }
    [[nodiscard]] inline int PopCount(Bitboard b) { return std::__popcount(b); }
    [[nodiscard]] inline Square Lsb(Bitboard b) { return static_cast<Square>(__builtin_ctzll(b)); }
    inline Square PopLsb(Bitboard &b) {
        Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

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

    inline Bitboard Shift(Direction shift_dir, Bitboard b) {
        if (shift_dir == NORTH) return Shift<NORTH>(b);
        else if (shift_dir == SOUTH) return Shift<SOUTH>(b);
        else if (shift_dir == EAST) return Shift<EAST>(b);
        else if (shift_dir == WEST) return Shift<WEST>(b);
        else if (shift_dir == NORTH_EAST) return Shift<NORTH_EAST>(b);
        else if (shift_dir == NORTH_WEST) return Shift<NORTH_WEST>(b);
        else if (shift_dir == SOUTH_EAST) return Shift<SOUTH_EAST>(b);
        else if (shift_dir == SOUTH_WEST) return Shift<SOUTH_WEST>(b);
        else return EMPTY_BB;
    }

    // win64 https://www.chessprogramming.org/BitScan -> Processor Instructions for Bitscans
    /*inline Square Lsb(Bitboard b) {
        unsigned long idx;
        _BitScanForward64(&idx, b);
        return (Square) idx;
    }*/

}

#endif //MEETRA_BITBOARDS_H
