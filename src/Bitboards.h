#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Defs.h"
#include <bit>
#include <array>

namespace Bitboards {

    constexpr std::array<Bitboard, RANK_NR> rank_mask{
            0x00000000000000FF,
            0x000000000000FF00,
            0x0000000000FF0000,
            0x00000000FF000000,
            0x000000FF00000000,
            0x0000FF0000000000,
            0x00FF000000000000,
            0xFF00000000000000
    };

    constexpr std::array<Bitboard, FILE_NR> file_mask{
            0x0101010101010101,
            0x0202020202020202,
            0x0404040404040404,
            0x0808080808080808,
            0x1010101010101010,
            0x2020202020202020,
            0x4040404040404040,
            0x8080808080808080
    };

    constexpr std::array<Bitboard, 15> diag_mask{
            0x1, 0x102, 0x10204, 0x1020408, 0x102040810, 0x10204081020, 0x1020408102040,
            0x102040810204080, 0x204081020408000, 0x408102040800000, 0x810204080000000,
            0x1020408000000000, 0x2040800000000000, 0x4080000000000000, 0x8000000000000000
    };

    constexpr std::array<Bitboard, 15> anti_diag_mask{
            0x80, 0x8040, 0x804020, 0x80402010, 0x8040201008, 0x804020100804, 0x80402010080402,
            0x8040201008040201, 0x4020100804020100, 0x2010080402010000, 0x1008040201000000,
            0x804020100000000, 0x402010000000000, 0x201000000000000, 0x100000000000000
    };

    constexpr std::array<Bitboard, COLOR_NR> castling_mask{
            rank_mask[RANK_1],
            rank_mask[RANK_8]
    };

    constexpr std::array<Bitboard, COLOR_NR> prom_mask{
            rank_mask[RANK_8],
            rank_mask[RANK_1]
    };

    constexpr std::array<Bitboard, COLOR_NR> two_fwd_mask{
            rank_mask[RANK_4],
            rank_mask[RANK_5]
    };

    struct Magic {
        Bitboard *attacks;
        Bitboard inner_mask;
        uint64_t magic_num;
        uint8_t shift;
    };

    inline std::array<Magic, SQUARE_NR> b_magics;
    inline std::array<Magic, SQUARE_NR> r_magics;

    inline std::array<Bitboard, 64> king_moves;
    inline std::array<Bitboard, 64> knight_moves;
    inline std::array<std::array<Bitboard, SQUARE_NR>, COLOR_NR> pawn_attacks;

    inline std::array<std::array<Bitboard, SQUARE_NR>, SQUARE_NR> rays_to_squares;
    inline std::array<std::array<Bitboard, SQUARE_NR>, SQUARE_NR> rays_to_borders;

    inline std::array<Bitboard, 88064> r_table;
    inline std::array<Bitboard, 4800> b_table;

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
