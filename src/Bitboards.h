#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Defs.h"
#include <bit>

namespace Bitboards {

    constexpr Bitboard rank_mask[RANK_NR]{
            0x00000000000000FF,
            0x000000000000FF00,
            0x0000000000FF0000,
            0x00000000FF000000,
            0x000000FF00000000,
            0x0000FF0000000000,
            0x00FF000000000000,
            0xFF00000000000000
    };

    constexpr Bitboard file_mask[FILE_NR]{
            0x0101010101010101,
            0x0202020202020202,
            0x0404040404040404,
            0x0808080808080808,
            0x1010101010101010,
            0x2020202020202020,
            0x4040404040404040,
            0x8080808080808080
    };

    constexpr Bitboard diag_mask[15]{
            0x1, 0x102, 0x10204, 0x1020408, 0x102040810, 0x10204081020, 0x1020408102040,
            0x102040810204080, 0x204081020408000, 0x408102040800000, 0x810204080000000,
            0x1020408000000000, 0x2040800000000000, 0x4080000000000000, 0x8000000000000000
    };

    constexpr Bitboard anti_diag_mask[15]{
            0x80, 0x8040, 0x804020, 0x80402010, 0x8040201008, 0x804020100804, 0x80402010080402,
            0x8040201008040201, 0x4020100804020100, 0x2010080402010000, 0x1008040201000000,
            0x804020100000000, 0x402010000000000, 0x201000000000000, 0x100000000000000
    };

    constexpr Bitboard castling_mask[COLOR_NR]{
            rank_mask[RANK_1],
            rank_mask[RANK_8]
    };

    constexpr Bitboard prom_mask[COLOR_NR]{
            rank_mask[RANK_8],
            rank_mask[RANK_1]
    };

    constexpr Bitboard two_fwd_mask[COLOR_NR]{
            rank_mask[RANK_4],
            rank_mask[RANK_5]
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

    [[nodiscard]] constexpr bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] constexpr bool ExactlyOne(Bitboard b) { return b && !MoreThanOne(b); }
    [[nodiscard]] constexpr Square Msb(Bitboard b) { return static_cast<Square>(63 ^ std::countl_zero(b)); }
    [[nodiscard]] constexpr Square Lsb(Bitboard b) { return static_cast<Square>(std::countr_zero(b)); }
    constexpr Square PopLsb(Bitboard &b) {
        Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    [[maybe_unused]] [[nodiscard]] std::string PPBitboard(Bitboard b);

    inline Bitboard GetRayToBorders(Square s1, Square s2) { return rays_to_borders[s1][s2]; }
    inline Bitboard GetRayToSquares(Square s1, Square s2) { return rays_to_squares[s1][s2]; }
    constexpr Bitboard RankMask(Rank r) { return rank_mask[r]; }
    constexpr Bitboard GetRookRays(Square s) { return file_mask[SqToFile(s)] | rank_mask[SqToRank(s)]; }

    constexpr Bitboard GetBishopRays(Square s) {
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        return diag_mask[f + r] | anti_diag_mask[r + 7 - f];
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
    [[nodiscard]] Bitboard GetAttacks(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE) {
        return PT == PAWN ? pawn_attacks[c][s] :
               PT == BISHOP ? GetBishopAttacks(s, occ) :
               PT == ROOK ? GetRookAttacks(s, occ) :
               PT == QUEEN ? GetBishopAttacks(s, occ) | GetRookAttacks(s, occ) :
               PT == KNIGHT ? knight_moves[s] :
               PT == KING ? king_moves[s] :
               EMPTY_BB;
    }

    template<Direction D>
    constexpr Bitboard Shift(Bitboard b) {
        return D == NORTH ? b << 8 :
               D == SOUTH ? b >> 8 :
               D == EAST ? (b & ~0x8080808080808080ULL) << 1 :
               D == WEST ? (b & ~0x0101010101010101ULL) >> 1 :
               D == NORTH_EAST ? (b & ~0x8080808080808080ULL) << 9 :
               D == NORTH_WEST ? (b & ~0x0101010101010101ULL) << 7 :
               D == SOUTH_EAST ? (b & ~0x8080808080808080ULL) >> 7 :
               D == SOUTH_WEST ? (b & ~0x0101010101010101ULL) >> 9 :
               EMPTY_BB;
    }
}

#endif //MEETRA_BITBOARDS_H
