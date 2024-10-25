#include "Bitboards.h"
#include "MagicNumbers.h"

#include <algorithm>
#include <sstream>
#include <ranges>
#include <functional>

namespace Bitboards {

    static Bitboard GenSliderMoves(Square s, Bitboard occ, const std::vector<Direction> &dirs);
    static Bitboard GenBishopMoves(Square s, Bitboard occ);
    static Bitboard GenRookMoves(Square s, Bitboard occ);
    static void GenPieceMoves(const std::vector<Direction> &dirs, std::array<Bitboard, SQUARE_NR> &output);
    static Bitboard GenRayToEdges(Square s1, Square s2);
    static Bitboard GenRayBetween(Square s1, Square s2);
    static void GenRays();
    static void InitMagic(std::array<BlackMagic, SQUARE_NR> &magics,
                          const std::array<MagicInit, SQUARE_NR> &magic_init, int shift,
                          const std::function<Bitboard(Square, Bitboard)> &generator);

    void Init() {
        GenRays();
        InitMagic(r_magics, r_init_magic, 64 - 12, GenRookMoves);
        InitMagic(b_magics, b_init_magic, 64 - 9, GenBishopMoves);
        GenPieceMoves({NORTH_EAST, NORTH_WEST}, pawn_attacks[WHITE]);
        GenPieceMoves({SOUTH_EAST, SOUTH_WEST}, pawn_attacks[BLACK]);
        GenPieceMoves({NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST}, king_moves);
        GenPieceMoves({NORTH + 2 * EAST, 2 * NORTH + EAST, NORTH + 2 * WEST, 2 * NORTH + WEST,
                       SOUTH + 2 * EAST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, 2 * SOUTH + WEST}, knight_moves);
    }

    [[maybe_unused]] std::string PPBitboard(Bitboard b) {
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

    static Bitboard GenSliderMoves(Square s, Bitboard occ, const std::vector<Direction> &dirs) {

        auto MoveOk = [](Square from, Square to) {
            int f_diff = std::abs(SqToFile(from) - SqToFile(to));
            int r_diff = std::abs(SqToRank(from) - SqToRank(to));
            return f_diff <= 1 && r_diff <= 1 && to >= A1 && to <= H8;
        };

        Bitboard attacks = EMPTY_BB;
        for (Direction d: dirs) {
            Square from = s;
            Square to = s + d;
            while (!(occ & SqToBB(from)) && MoveOk(from, to)) {
                attacks |= SqToBB(to);
                from = to;
                to += d;
            }
        }
        return attacks;
    }

    static Bitboard GenBishopMoves(Square s, Bitboard occ) {
        return GenSliderMoves(s, occ, {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST});
    }

    static Bitboard GenRookMoves(Square s, Bitboard occ) {
        return GenSliderMoves(s, occ, {NORTH, SOUTH, EAST, WEST});
    }

    static void GenPieceMoves(const std::vector<Direction> &dirs, std::array<Bitboard, SQUARE_NR> &output) {
        for (Square s: Squares) {
            output[s] = EMPTY_BB;
            for (Direction d: dirs) {
                bool square_is_on_board = s + d < SQUARE_NR && s + d >= A1;
                if (square_is_on_board) {
                    output[s] |= SqToBB(s + d);
                }
            }
            File f = SqToFile(s);
            output[s] &= f > FILE_D ? ~file_mask[FILE_A] & ~file_mask[FILE_B] : ~file_mask[FILE_G] & ~file_mask[FILE_H];
        }
    }

    static Bitboard GenRayToEdges(Square s1, Square s2) {

        File f1 = SqToFile(s1);
        Rank r1 = SqToRank(s1);
        File f2 = SqToFile(s2);
        Rank r2 = SqToRank(s2);

        return s1 == s2 ? EMPTY_BB :
               r1 == r2 ? rank_mask[r1] :
               f1 == f2 ? file_mask[f1] :
               f1 + r1 == f2 + r2 ? diag_mask[f1 + r1] :
               f1 - r1 == f2 - r2 ? anti_diag_mask[r1 + 7 - f1] :
               EMPTY_BB;
    }

    static Bitboard GenRayBetween(Square s1, Square s2) {

        auto [s_min, s_max] = std::minmax(s1, s2);
        Rank r_max = SqToRank(s_max);
        File f_max = SqToFile(s_max);
        Rank r_min = SqToRank(s_min);
        File f_min = SqToFile(s_min);

        Bitboard mask = SqToBB(s_max) - (SqToBB(s_min) << 1);

        return s1 == s2 ? EMPTY_BB :
               r_max == r_min ? rank_mask[r_max] & mask :
               f_max == f_min ? file_mask[f_max] & mask :
               f_min + r_min == f_max + r_max ? diag_mask[f_max + r_max] & mask :
               f_min - r_min == f_max - r_max ? anti_diag_mask[r_max + 7 - f_max] & mask :
               EMPTY_BB;
    }

    static void GenRays() {
        for (Square s1: Squares) {
            for (Square s2: Squares) {
                rays_to_squares[s1][s2] = GenRayBetween(s1, s2);
                rays_to_borders[s1][s2] = GenRayToEdges(s1, s2);
            }
        }
    }

    static void InitMagic(std::array<BlackMagic, SQUARE_NR> &magics,
                          const std::array<MagicInit, SQUARE_NR> &magic_init, int shift,
                          const std::function<Bitboard(Square, Bitboard)> &generator) {
        for (Square s: Squares) {
            Bitboard rank_edge_mask = (rank_mask[RANK_1] | rank_mask[RANK_8]) & ~rank_mask[SqToRank(s)];
            Bitboard file_edge_mask = (file_mask[FILE_A] | file_mask[FILE_H]) & ~file_mask[SqToFile(s)];
            Bitboard inner_mask = ~(rank_edge_mask | file_edge_mask);
            magics[s].mask = ~(inner_mask & generator(s, EMPTY_BB));
            magics[s].magic_num = magic_init[s].factor;
            magics[s].attacks = &magic_lookup[magic_init[s].position];
            Bitboard occ = EMPTY_BB;
            do {
                auto idx = ((occ | magics[s].mask) * magics[s].magic_num) >> shift;
                magics[s].attacks[idx] = generator(s, occ);
                occ = (occ - (~magics[s].mask)) & (~magics[s].mask);
            } while (occ);
        }
    }
}
