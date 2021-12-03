#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Defs.h"
#include <bit>

namespace Bitboards {

    constexpr Bitboard rank_masks[RANK_NR]{
            0x00000000000000FF,
            0x000000000000FF00,
            0x0000000000FF0000,
            0x00000000FF000000,
            0x000000FF00000000,
            0x0000FF0000000000,
            0x00FF000000000000,
            0xFF00000000000000
    };

    constexpr Bitboard file_masks[FILE_NR]{
            0x0101010101010101,
            0x0202020202020202,
            0x0404040404040404,
            0x0808080808080808,
            0x1010101010101010,
            0x2020202020202020,
            0x4040404040404040,
            0x8080808080808080
    };

    constexpr Bitboard diag_masks[15]{
            0x1, 0x102, 0x10204, 0x1020408, 0x102040810, 0x10204081020, 0x1020408102040,
            0x102040810204080, 0x204081020408000, 0x408102040800000, 0x810204080000000,
            0x1020408000000000, 0x2040800000000000, 0x4080000000000000, 0x8000000000000000
    };

    constexpr Bitboard anti_diag_masks[15]{
            0x80, 0x8040, 0x804020, 0x80402010, 0x8040201008, 0x804020100804, 0x80402010080402,
            0x8040201008040201, 0x4020100804020100, 0x2010080402010000, 0x1008040201000000,
            0x804020100000000, 0x402010000000000, 0x201000000000000, 0x100000000000000
    };

    struct Magic {
        Bitboard *attacks;
        Bitboard inner_mask;
        uint64_t magic_num;
        uint8_t shift;
    };

    inline Magic b_magics[SQUARE_NR];
    inline Magic r_magics[SQUARE_NR];

    inline Bitboard king_moves[SQUARE_NR];
    inline Bitboard knight_moves[SQUARE_NR];
    inline Bitboard pawn_attacks[COLOR_NR][SQUARE_NR];

    inline Bitboard rays_to_squares[SQUARE_NR][SQUARE_NR];
    inline Bitboard rays_to_borders[SQUARE_NR][SQUARE_NR];

    inline Bitboard r_table[88064];
    inline Bitboard b_table[4800];


    void Init();

    template<PieceType PT>
    [[nodiscard]] Bitboard GetAttacks(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE);

    [[nodiscard]] constexpr bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] constexpr bool ExactlyOne(Bitboard b) { return b && !MoreThanOne(b); }
    [[nodiscard]] constexpr Square Msb(Bitboard b) { return 63 ^ std::countl_zero(b); }
    [[nodiscard]] constexpr Square Lsb(Bitboard b) { return std::countr_zero(b); }
    constexpr Square PopLsb(Bitboard &b) {
        Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    [[maybe_unused]] [[nodiscard]] std::string PPBitboard(Bitboard b);

    inline Bitboard GetRayToBorders(Square s1, Square s2) { return rays_to_borders[s1][s2]; }
    inline Bitboard GetRayToSquares(Square s1, Square s2) { return rays_to_squares[s1][s2]; }
    constexpr Bitboard RankMask(Rank r) { return rank_masks[r]; }
    constexpr Bitboard GetRookRays(Square s) { return file_masks[SqToFile(s)] | rank_masks[SqToRank(s)]; }

    constexpr Bitboard GetBishopRays(Square s) {
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        return diag_masks[f + r] | anti_diag_masks[r + 7 - f];
    }

    inline Bitboard GetRookAttacks(Square s, Bitboard occ) {
        Magic m = r_magics[s];
        return m.attacks[((occ & m.inner_mask) * m.magic_num) >> m.shift];
    }

    inline Bitboard GetBishopAttacks(Square s, Bitboard occ) {
        Magic m = b_magics[s];
        return m.attacks[((occ & m.inner_mask) * m.magic_num) >> m.shift];
    }

    template<PieceType PT>
    Bitboard GetAttacks(Square s, Bitboard occ, Color c) {
        if constexpr (PT == PAWN) return pawn_attacks[c][s];
        else if constexpr (PT == BISHOP) return GetBishopAttacks(s, occ);
        else if constexpr (PT == ROOK) return GetRookAttacks(s, occ);
        else if constexpr (PT == QUEEN) return GetBishopAttacks(s, occ) | GetRookAttacks(s, occ);
        else if constexpr (PT == KNIGHT) return knight_moves[s];
        else if constexpr (PT == KING) return king_moves[s];
        else return EMPTY_BB;
    }

    template<Direction D>
    constexpr Bitboard Shift(Bitboard b) {
        if constexpr (D == NORTH) return b << 8;
        else if constexpr (D == SOUTH) return b >> 8;
        else if constexpr (D == EAST) return (b & ~0x8080808080808080ULL) << 1;
        else if constexpr (D == WEST) return (b & ~0x0101010101010101ULL) >> 1;
        else if constexpr (D == NORTH_EAST) return (b & ~0x8080808080808080ULL) << 9;
        else if constexpr (D == NORTH_WEST) return (b & ~0x0101010101010101ULL) << 7;
        else if constexpr (D == SOUTH_EAST) return (b & ~0x8080808080808080ULL) >> 7;
        else if constexpr (D == SOUTH_WEST) return (b & ~0x0101010101010101ULL) >> 9;
        else return EMPTY_BB;
    }
}

#endif //MEETRA_BITBOARDS_H
