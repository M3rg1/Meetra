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

    constexpr std::array castling_mask{
            rank_mask[RANK_1],
            rank_mask[RANK_8]
    };

    constexpr std::array prom_mask{
            rank_mask[RANK_8],
            rank_mask[RANK_1]
    };

    constexpr std::array two_fwd_mask{
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

    inline std::array<Bitboard, 88064> r_table;
    inline std::array<Bitboard, 4800> b_table;

    constexpr Bitboard GenRayToEdge(Square s1, Square s2) {

        if (s1 == s2) {
            return EMPTY_BB;
        }

        File f1 = SqToFile(s1);
        Rank r1 = SqToRank(s1);
        File f2 = SqToFile(s2);
        Rank r2 = SqToRank(s2);

        return r1 == r2 ? rank_mask[r1] :
               f1 == f2 ? file_mask[f1] :
               f1 + r1 == f2 + r2 ? diag_mask[f1 + r1] :
               f1 - r1 == f2 - r2 ? anti_diag_mask[r1 + 7 - f1] :
               EMPTY_BB;
    }

    constexpr Bitboard GenRay(Square s1, Square s2) {

        if (s1 == s2) {
            return EMPTY_BB;
        }

        Square max = std::max(s1, s2);
        Square min = std::min(s1, s2);

        Rank r_max = SqToRank(max);
        File f_max = SqToFile(max);

        Rank r_min = SqToRank(min);
        File f_min = SqToFile(min);

        Bitboard mask = SqToBB(max) - (SqToBB(min) << 1);

        return r_max == r_min ? rank_mask[r_max] & mask :
               f_max == f_min ? file_mask[f_max] & mask :
               f_min + r_min == f_max + r_max ? diag_mask[f_max + r_max] & mask :
               f_min - r_min == f_max - r_max ? anti_diag_mask[r_max + 7 - f_max] & mask :
               EMPTY_BB;
    }

    consteval auto GenRaysBetweenSquares(Bitboard (*RayGen)(Square, Square)) {
        std::array<std::array<Bitboard, SQUARE_NR>, SQUARE_NR> arr{};
        for (Square s1: Squares) {
            for (Square s2: Squares) {
                arr[s1][s2] = RayGen(s1, s2);
            }
        }
        return arr;
    }

    constexpr auto rays_to_squares = GenRaysBetweenSquares(GenRay);
    constexpr auto rays_to_borders = GenRaysBetweenSquares(GenRayToEdge);

    consteval auto GenPieceMoves(std::initializer_list<Direction> dirs) {
        std::array<Bitboard, SQUARE_NR> moves{};
        for (Square s: Squares) {
            for (const auto &d: dirs) {
                if (s + d < SQUARE_NR && s + d >= A1) {
                    moves[s] |= SqToBB(s + d);
                }
            }
            File f = SqToFile(s);
            moves[s] &= f > FILE_D ? ~file_mask[FILE_A] & ~file_mask[FILE_B] : ~file_mask[FILE_G] & ~file_mask[FILE_H];
        }
        return moves;
    }

    constexpr auto king_moves = GenPieceMoves(
            {NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST});
    constexpr auto knight_moves = GenPieceMoves({NORTH + 2 * EAST, 2 * NORTH + EAST, NORTH + 2 * WEST, 2 * NORTH + WEST,
                                                 SOUTH + 2 * EAST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, 2 * SOUTH + WEST}
    );

    constexpr std::array pawn_attacks = {
            GenPieceMoves({NORTH_EAST, NORTH_WEST}),
            GenPieceMoves({SOUTH_EAST, SOUTH_WEST})
    };

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
