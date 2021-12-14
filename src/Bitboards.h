#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Defs.h"
#include <bit>
#include <array>
#include <sstream>
#include <ranges>
#include "MagicNumbers.h"

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
        const Bitboard *attacks;
        Bitboard inner_mask;
        uint64_t magic_num;
        uint8_t shift;
    };

    constexpr Bitboard ReverseBits(Bitboard b) {
        b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
        b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
        b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);
        b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b & 0x00FF00FF00FF00FFULL) << 8);
        b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b & 0x0000FFFF0000FFFFULL) << 16);
        b = (b >> 32) | (b << 32);
        return b;
    }

    constexpr Bitboard GenDiagMoves(Square s, Bitboard occ) {
        Bitboard bitboard = SqToBB(s);
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        Bitboard move_mask = anti_diag_mask[r + 7 - f];
        return (((occ & move_mask) - (bitboard << 1))
                ^ ReverseBits(ReverseBits(occ & move_mask) - (ReverseBits(bitboard) << 1))) & move_mask;
    }

    constexpr Bitboard GenAntiDiagMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        Bitboard move_mask = diag_mask[f + r];
        return (((occ & move_mask) - (b << 1))
                ^ ReverseBits(ReverseBits(occ & move_mask) - (ReverseBits(b) << 1))) & move_mask;
    }

    constexpr Bitboard GenVerticalMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        File f = SqToFile(s);
        return (((occ & file_mask[f]) - (b << 1))
                ^ ReverseBits(ReverseBits(occ & file_mask[f]) - (ReverseBits(b) << 1))) & file_mask[f];
    }

    constexpr Bitboard GenHorizontalMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        Rank r = SqToRank(s);
        return ((occ - (b << 1)) ^ ReverseBits(ReverseBits(occ) - (ReverseBits(b) << 1))) & rank_mask[r];
    }


    /// safe_destination() returns the bitboard of target square for the given step
/// from the given square. If the step is off the board, returns empty bitboard.

    constexpr int distance_f(Square x, Square y) { return std::abs(SqToFile(x) - SqToFile(y)); }
    constexpr int distance_r(Square x, Square y) { return std::abs(SqToRank(x) - SqToRank(y)); }
    constexpr int distance_s(Square x, Square y) { return std::max(distance_f(x, y), distance_r(x, y)); }

    constexpr bool is_ok(Square s) {
        return s >= A1 && s <= H8;
    }

    constexpr Bitboard safe_destination(Square s, Direction dir) {
        Square to = s + dir;
        return is_ok(to) && distance_s(s, to) <= 2 ? SqToBB(to) : EMPTY_BB;
    }

    constexpr Bitboard sliding_attack(Square sq, Bitboard occupied, std::initializer_list<Direction> dirs) {
        Bitboard attacks = EMPTY_BB;
        for (Direction d : dirs) {
            Square s = sq;
            while (safe_destination(s, d) && !(occupied & SqToBB(s))) {
                attacks |= SqToBB(s += d);
            }
        }
        return attacks;
    }

    constexpr Bitboard GenBishopMoves(Square s, Bitboard occ) {
        //return GenAntiDiagMoves(s, occ) | GenDiagMoves(s, occ);
        return sliding_attack(s, occ, {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST});
    }

    constexpr Bitboard GenRookMoves(Square s, Bitboard occ) {
        //return GenVerticalMoves(s, occ) | GenHorizontalMoves(s, occ);
        return sliding_attack(s, occ, {NORTH, SOUTH, EAST, WEST});
    }

    template<auto size>
    consteval auto GenTable(auto &magic_shift, auto &magic_num, auto &generator) {
        std::array<Bitboard, size> table{};
        for (auto curr = table.data(); Square s: Squares) {
            Bitboard occ = EMPTY_BB;
            Bitboard inner = ~(((rank_mask[RANK_1] | rank_mask[RANK_8]) & ~rank_mask[SqToRank(s)])
                               | ((file_mask[FILE_A] | file_mask[FILE_H]) & ~file_mask[SqToFile(s)]))
                             & generator(s, EMPTY_BB);
            do {
                auto idx = ((occ & inner) * magic_num[s]) >> (64 - magic_shift[s]);
                curr[idx] = generator(s, occ);
                occ = (occ - inner) & inner;
            } while (occ);
            curr += 1 << magic_shift[s];
        }
        return table;
    }

    constexpr auto r_table = GenTable<88064>(r_magic_shift, r_magic_num, GenRookMoves);
    constexpr auto b_table = GenTable<4800>(b_magic_shift, b_magic_num, GenBishopMoves);

    consteval auto InitMagic(auto &table, auto &magic_shift, auto &magic_num, auto &generator) {
        std::array<Magic, SQUARE_NR> magics{};
        for (Square s: Squares) {
            Bitboard inner = ((rank_mask[RANK_1] | rank_mask[RANK_8]) & ~rank_mask[SqToRank(s)])
                             | ((file_mask[FILE_A] | file_mask[FILE_H]) & ~file_mask[SqToFile(s)]);
            magics[s].shift = 64 - magic_shift[s];
            magics[s].inner_mask = generator(s, EMPTY_BB) & ~inner;
            magics[s].magic_num = magic_num[s];
            magics[s].attacks = s == A1 ? table.data() : magics[s - 1].attacks + (1 << magic_shift[s - 1]);
        }
        return magics;
    }

    constexpr auto r_magics = InitMagic(r_table, r_magic_shift, r_magic_num, GenRookMoves);
    constexpr auto b_magics = InitMagic(b_table, b_magic_shift, b_magic_num, GenBishopMoves);

    constexpr Bitboard GenRaysToEdge(Square s1, Square s2) {

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

    constexpr Bitboard GenRayBetweenSquares(Square s1, Square s2) {

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

    consteval auto GenRays(Bitboard (*RayGen)(Square, Square)) {
        std::array<std::array<Bitboard, SQUARE_NR>, SQUARE_NR> arr{};
        for (Square s1: Squares) {
            for (Square s2: Squares) {
                arr[s1][s2] = RayGen(s1, s2);
            }
        }
        return arr;
    }

    constexpr auto rays_to_squares = GenRays(GenRayBetweenSquares);
    constexpr auto rays_to_borders = GenRays(GenRaysToEdge);

    consteval auto GenPieceMoves(std::initializer_list<Direction> dirs) {
        std::array<Bitboard, SQUARE_NR> moves{};
        for (Square s: Squares) {
            for (const auto &d: dirs) {
                if (s + d < SQUARE_NR && s + d >= A1) {
                    moves[s] |= SqToBB(s + d);
                }
            }
            File f = SqToFile(s);
            moves[s] &=
                    f > FILE_D ? ~file_mask[FILE_A] & ~file_mask[FILE_B] : ~file_mask[FILE_G] & ~file_mask[FILE_H];
        }
        return moves;
    }

    constexpr auto king_moves = GenPieceMoves(
            {NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST});

    constexpr auto knight_moves = GenPieceMoves(
            {NORTH + 2 * EAST, 2 * NORTH + EAST, NORTH + 2 * WEST, 2 * NORTH + WEST,
             SOUTH + 2 * EAST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, 2 * SOUTH + WEST});

    constexpr std::array pawn_attacks = {
            GenPieceMoves({NORTH_EAST, NORTH_WEST}),
            GenPieceMoves({SOUTH_EAST, SOUTH_WEST})
    };

    constexpr Bitboard GetRayToBorders(Square s1, Square s2) { return rays_to_borders[s1][s2]; }
    constexpr Bitboard GetRayToSquares(Square s1, Square s2) { return rays_to_squares[s1][s2]; }

    constexpr Bitboard GetRookAttacks(Square s, Bitboard occ) {
        Magic m = r_magics[s];
        return m.attacks[((occ & m.inner_mask) * m.magic_num) >> m.shift];
    }

    constexpr Bitboard GetBishopAttacks(Square s, Bitboard occ) {
        Magic m = b_magics[s];
        return m.attacks[((occ & m.inner_mask) * m.magic_num) >> m.shift];
    }

    template<PieceType PT>
    [[nodiscard]] constexpr Bitboard GetAttacks(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE) {
        return PT == PAWN ? pawn_attacks[c][s] :
               PT == BISHOP ? GetBishopAttacks(s, occ) :
               PT == ROOK ? GetRookAttacks(s, occ) :
               PT == QUEEN ? GetBishopAttacks(s, occ) | GetRookAttacks(s, occ) :
               PT == KNIGHT ? knight_moves[s] :
               PT == KING ? king_moves[s] :
               EMPTY_BB;
    }

    constexpr Bitboard GetRookRays(Square s) { return file_mask[SqToFile(s)] | rank_mask[SqToRank(s)]; }

    constexpr Bitboard GetBishopRays(Square s) {
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        return diag_mask[f + r] | anti_diag_mask[r + 7 - f];
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

    [[nodiscard]] constexpr bool MoreThanOne(Bitboard b) { return (b & (b - 1)); }
    [[nodiscard]] constexpr bool ExactlyOne(Bitboard b) { return b && !MoreThanOne(b); }
    [[nodiscard]] constexpr Square Msb(Bitboard b) { return static_cast<Square>(63 ^ std::countl_zero(b)); }
    [[nodiscard]] constexpr Square Lsb(Bitboard b) { return static_cast<Square>(std::countr_zero(b)); }
    constexpr Square PopLsb(Bitboard &b) {
        Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    [[maybe_unused]] [[nodiscard]] inline std::string PPBitboard(Bitboard b) {
        std::ostringstream oss;
        for (Rank r: Ranks | std::views::reverse) {
            oss << r + 1 << " |";
            for (File f: Files) {
                if ((b >> ((r * 8) + f)) & 1) {
                    oss << " x ";
                } else {
                    oss << " o ";
                }
            }
            oss << '\n';
        }
        oss << "    A  B  C  D  E  F  G  H";
        return oss.str();
    }
}

#endif //MEETRA_BITBOARDS_H
