#include "Bitboards.h"
#include "MagicNumbers.h"
#include <sstream>
#include <algorithm>
#include <ranges>

namespace Bitboards {

#pragma region ===== Hyperbola Quintessence (used for magics initialization) =====

    Bitboard ReverseBits(Bitboard b) {
        b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
        b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
        b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);
        b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b & 0x00FF00FF00FF00FFULL) << 8);
        b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b & 0x0000FFFF0000FFFFULL) << 16);
        b = (b >> 32) | (b << 32);
        return b;
    }

    Bitboard GenDiagMoves(Square s, Bitboard occ) {
        Bitboard bitboard = SqToBB(s);
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        Bitboard move_mask = anti_diag_mask[r + 7 - f];
        return (((occ & move_mask) - (bitboard << 1)) ^ ReverseBits(ReverseBits(occ & move_mask)
                                                                    - (ReverseBits(bitboard) << 1))) & move_mask;
    }

    Bitboard GenAntiDiagMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        File f = SqToFile(s);
        Rank r = SqToRank(s);
        Bitboard move_mask = diag_mask[f + r];
        return (((occ & move_mask) - (b << 1))
                ^ ReverseBits(ReverseBits(occ & move_mask) - (ReverseBits(b) << 1))) & move_mask;
    }

    Bitboard GenHorizontalMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        Rank r = SqToRank(s);
        return ((occ - (b << 1)) ^ ReverseBits(ReverseBits(occ) - (ReverseBits(b) << 1))) & rank_mask[r];
    }

    Bitboard GenVerticalMoves(Square s, Bitboard occ) {
        Bitboard b = SqToBB(s);
        File f = SqToFile(s);
        return (((occ & file_mask[f]) - (b << 1))
                ^ ReverseBits(ReverseBits(occ & file_mask[f]) - (ReverseBits(b) << 1))) & file_mask[f];
    }

    Bitboard GenBishopMoves(Square s, Bitboard occ) {
        return GenAntiDiagMoves(s, occ) | GenDiagMoves(s, occ);
    }

    Bitboard GenRookMoves(Square s, Bitboard occ) {
        return GenVerticalMoves(s, occ) | GenHorizontalMoves(s, occ);
    }

#pragma endregion

#pragma region ===== Fancy Magic Bitboards initialization =====

    void SetBlockersRecursive(Magic &m, Square origin, Bitboard blockers, Bitboard explore_occ,
                              Bitboard (*move_generator)(Square, Bitboard)) {

        if (explore_occ == EMPTY_BB) {
            auto idx = ((blockers & m.inner_mask) * m.magic_num) >> m.shift;
            m.attacks[idx] = move_generator(origin, blockers);
            return;
        }

        Square current = PopLsb(explore_occ);

        blockers |= SqToBB(current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);

        blockers ^= SqToBB(current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);
    }

    void GenMagics() {
        for (Square s: Squares) {
            SetBlockersRecursive(r_magics[s], s, EMPTY_BB, GetRookRays(s), GenRookMoves);
            SetBlockersRecursive(b_magics[s], s, EMPTY_BB, GetBishopRays(s), GenBishopMoves);
        }
    }

    void InitMagic() {
        for (Square s: Squares) {

            Bitboard inner = (GenHorizontalMoves(s, EMPTY_BB) & ~file_mask[FILE_A] & ~file_mask[FILE_H])
                             | (GenVerticalMoves(s, EMPTY_BB) & ~rank_mask[RANK_1] & ~rank_mask[RANK_8]);

            r_magics[s].shift = 64 - r_magic_shift[s];
            r_magics[s].inner_mask = inner;
            r_magics[s].magic_num = rook_magic_num[s];
            r_magics[s].attacks = s == A1 ? r_table : r_magics[s - 1].attacks + (1 << r_magic_shift[s - 1]);

            inner = GenBishopMoves(s, EMPTY_BB) & ~file_mask[FILE_A] & ~rank_mask[RANK_1] & ~file_mask[FILE_H]
                    & ~rank_mask[RANK_8];

            b_magics[s].shift = 64 - b_magic_shift[s];
            b_magics[s].inner_mask = inner;
            b_magics[s].magic_num = bishop_magic_num[s];
            b_magics[s].attacks = s == A1 ? b_table : b_magics[s - 1].attacks + (1 << b_magic_shift[s - 1]);
        }
    }

#pragma endregion

#pragma region ===== Precomputing king moves, knight moves, pawn attacks =====

    void GenPieceMoves(std::initializer_list<Direction> dirs, Bitboard output[SQUARE_NR]) {
        for (Square s: Squares) {
            Bitboard moves = EMPTY_BB;
            for (auto d: dirs) {
                if (s + d < SQUARE_NR && s + d >= A1) {
                    moves |= SqToBB(s + d);
                }
            }
            File f = SqToFile(s);
            moves &= f > FILE_D ? ~file_mask[FILE_A] & ~file_mask[FILE_B] : ~file_mask[FILE_G] & ~file_mask[FILE_H];
            output[s] = moves;
        }
    }

#pragma endregion

#pragma region ===== Generate helper rays =====

    Bitboard GenRayToEdge(Square s1, Square s2) {

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

    Bitboard GenRay(Square s1, Square s2) {

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

    void GenRaysBetweenSquares() {
        for (Square s1: Squares) {
            for (Square s2: Squares) {
                rays_to_squares[s1][s2] = GenRay(s1, s2);
                rays_to_borders[s1][s2] = GenRayToEdge(s1, s2);
            }
        }
    }

#pragma enregion

#pragma region ===== Misc =====

    void Init() {
        GenRaysBetweenSquares();
        InitMagic();
        // rook + bishop (+ queen) moves
        GenMagics();
        // white pawns
        GenPieceMoves({NORTH_EAST, NORTH_WEST}, pawn_attacks[WHITE]);
        // black pawns
        GenPieceMoves({SOUTH_EAST, SOUTH_WEST}, pawn_attacks[BLACK]);
        // kings
        GenPieceMoves({NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST}, king_moves);
        // knights
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

#pragma endregion
}

