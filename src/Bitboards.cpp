#include "Bitboards.h"
#include "MagicNumbers.h"
#include <sstream>

namespace Meetra::Bitboards {

    constexpr Bitboard rank_masks[RANK_NR]{
            0x00000000000000FFUL,
            0x000000000000FF00UL,
            0x0000000000FF0000UL,
            0x00000000FF000000UL,
            0x000000FF00000000UL,
            0x0000FF0000000000UL,
            0x00FF000000000000UL,
            0xFF00000000000000UL
    };

    constexpr Bitboard file_masks[FILE_NR]{
            0x0101010101010101UL,
            0x0202020202020202UL,
            0x0404040404040404UL,
            0x0808080808080808UL,
            0x1010101010101010UL,
            0x2020202020202020UL,
            0x4040404040404040UL,
            0x8080808080808080UL
    };

    constexpr Bitboard diag_masks[15]{
            0x1L, 0x102L, 0x10204L, 0x1020408L, 0x102040810L, 0x10204081020L, 0x1020408102040L,
            0x102040810204080L, 0x204081020408000L, 0x408102040800000L, 0x810204080000000L,
            0x1020408000000000L, 0x2040800000000000L, 0x4080000000000000L, 0x8000000000000000L
    };

    constexpr Bitboard anti_diag_masks[15]{
            0x80L, 0x8040L, 0x804020L, 0x80402010L, 0x8040201008L, 0x804020100804L, 0x80402010080402L,
            0x8040201008040201L, 0x4020100804020100L, 0x2010080402010000L, 0x1008040201000000L,
            0x804020100000000L, 0x402010000000000L, 0x201000000000000L, 0x100000000000000L
    };

    struct Magic {
        Bitboard *attacks;
        Bitboard inner_board_mask;
        uint64_t magic_num;
        uint8_t shift;
    };

    Magic bishop_magics[SQUARE_NR];
    Magic rook_magics[SQUARE_NR];

    Bitboard king_moves[SQUARE_NR];
    Bitboard knight_moves[SQUARE_NR];

    Bitboard pawn_attacks[COLOR_NR][SQUARE_NR];

    Bitboard rays[SQUARE_NR][DIRECTION_IDX_NR];
    Bitboard inner_rays[SQUARE_NR][DIRECTION_IDX_NR];

    Bitboard rays_between_squares[SQUARE_NR][SQUARE_NR];
    Bitboard rays_between_board_edges[SQUARE_NR][SQUARE_NR];

    Bitboard rook_unbound_moves[SQUARE_NR];
    Bitboard bishop_unbound_moves[SQUARE_NR];

    Bitboard rook_table[88064];
    Bitboard bishop_table[4800];

    Bitboard GetUnboundRookMoves(Square s) { return rook_unbound_moves[s]; }
    Bitboard GetUnboundBishopMoves(Square s) { return bishop_unbound_moves[s]; }
    Bitboard GetRayBetweenEdges(Square s1, Square s2) { return rays_between_board_edges[s1][s2]; }
    Bitboard GetRayBetweenSquares(Square s1, Square s2) { return rays_between_squares[s1][s2]; }


#pragma region ===== Hyperbola Quintessence, Reverse Bitboards (for magics initialization) =====

    Bitboard ReverseBits(Bitboard b) {
        b = ((b >> 1) & 0x5555555555555555UL) | ((b & 0x5555555555555555UL) << 1);
        b = ((b >> 2) & 0x3333333333333333UL) | ((b & 0x3333333333333333UL) << 2);
        b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FUL) | ((b & 0x0F0F0F0F0F0F0F0FUL) << 4);
        b = ((b >> 8) & 0x00FF00FF00FF00FFUL) | ((b & 0x00FF00FF00FF00FFUL) << 8);
        b = ((b >> 16) & 0x0000FFFF0000FFFFUL) | ((b & 0x0000FFFF0000FFFFUL) << 16);
        b = (b >> 32) | (b << 32);
        return b;
    }

    Bitboard GetDiagMoves(Square s, Bitboard occ) {
        Bitboard bitboard = SquareToBB(s);
        Bitboard move_mask = rays[s][NORTH_EAST_IDX] | rays[s][SOUTH_WEST_IDX];
        return (((occ & move_mask) - (bitboard << 1)) ^ ReverseBits(ReverseBits(occ & move_mask)
                                                                    - (ReverseBits(bitboard) << 1))) & move_mask;
    }

    Bitboard GetAntiDiagMoves(Square s, Bitboard occ) {
        Bitboard b = SquareToBB(s);
        Bitboard move_mask = rays[s][NORTH_WEST_IDX] | rays[s][SOUTH_EAST_IDX];
        return (((occ & move_mask) - (b << 1)) ^
                ReverseBits(ReverseBits(occ & move_mask) - (ReverseBits(b) << 1))) & move_mask;
    }

    Bitboard GetHorizontalMoves(Square s, Bitboard occ) {
        Bitboard b = SquareToBB(s);
        Rank r = RankFromSquare(s);
        return ((occ - (b << 1)) ^ ReverseBits(ReverseBits(occ) - (ReverseBits(b) << 1))) & rank_masks[r];
    }

    Bitboard GetVerticalMoves(Square s, Bitboard occ) {
        Bitboard b = SquareToBB(s);
        File f = FileFromSquare(s);
        return (((occ & file_masks[f]) - (b << 1)) ^
                ReverseBits(ReverseBits(occ & file_masks[f]) - (ReverseBits(b) << 1))) & file_masks[f];
    }

    Bitboard GetDiagAndAntiDiagMoves(Square s, Bitboard occ) {
        return GetAntiDiagMoves(s, occ) | GetDiagMoves(s, occ);
    }

    Bitboard GetHorAndVertMoves(Square s, Bitboard occ) {
        return GetVerticalMoves(s, occ) | GetHorizontalMoves(s, occ);
    }
#pragma endregion


#pragma region ===== "Fancy" Magic Bitboards initialization =====

    void SetBlockersRecursive(Magic &m, Square origin, Bitboard blockers, Bitboard explore_occ,
                              Bitboard (*move_generator)(Square, Bitboard)) {

        if (explore_occ == EMPTY_BB) {
            auto idx = ((blockers & m.inner_board_mask) * m.magic_num) >> m.shift;
            m.attacks[idx] = move_generator(origin, blockers);
            return;
        }

        Square current = PopLsb(explore_occ);

        blockers |= SquareToBB(current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);

        blockers ^= SquareToBB(current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);

    }

    void FillMagics() {
        for (Square s = A1; s <= H8; ++s) {
            SetBlockersRecursive(rook_magics[s], s, EMPTY_BB, rook_unbound_moves[s], GetHorAndVertMoves);
            SetBlockersRecursive(bishop_magics[s], s, EMPTY_BB, bishop_unbound_moves[s], GetDiagAndAntiDiagMoves);
        }
    }

    void InitMagic() {
        for (Square s = A1; s <= H8; ++s) {
            rook_magics[s].shift = static_cast<uint8_t>(64 - rook_magic_shift[s]);
            rook_magics[s].inner_board_mask = inner_rays[s][NORTH_IDX] | inner_rays[s][EAST_IDX]
                                              | inner_rays[s][SOUTH_IDX] | inner_rays[s][WEST_IDX];
            rook_magics[s].magic_num = rook_magic_num[s];
            rook_magics[s].attacks = s == A1 ? rook_table : rook_magics[s - 1].attacks + (1 << rook_magic_shift[s - 1]);

            bishop_magics[s].shift = static_cast<uint8_t>(64 - bishop_magic_shift[s]);
            bishop_magics[s].inner_board_mask = inner_rays[s][NORTH_WEST_IDX] | inner_rays[s][NORTH_EAST_IDX]
                                                | inner_rays[s][SOUTH_EAST_IDX] | inner_rays[s][SOUTH_WEST_IDX];
            bishop_magics[s].magic_num = bishop_magic_num[s];
            bishop_magics[s].attacks =
                    s == A1 ? bishop_table : bishop_magics[s - 1].attacks + (1 << bishop_magic_shift[s - 1]);
        }
        FillMagics();
    }
#pragma endregion


#pragma region ===== Precomputing king, knight moves and pawn attacks =====

    void InitPawnAttacks() {
        int possible_moves[] = {7, 9};
        for (Square s = A1; s <= H8; ++s) {
            Bitboard attacks = EMPTY_BB;
            for (int m : possible_moves) {
                if (s + m <= H8 && s + m >= A1) {
                    attacks |= SquareToBB(s + m);
                }
            }
            attacks &= FileFromSquare(s) > FILE_C ? ~file_masks[FILE_A] : ~file_masks[FILE_H];
            pawn_attacks[WHITE][s] = attacks;

            attacks = EMPTY_BB;
            for (int m : possible_moves) {
                if (s - m <= H8 && s - m >= A1) {
                    attacks |= SquareToBB(s - m);
                }
            }
            attacks &= FileFromSquare(s) > FILE_C ? ~file_masks[FILE_A] : ~file_masks[FILE_H];
            pawn_attacks[BLACK][s] = attacks;
        }
    }

    void InitKingMoves() {
        for (Square s = A1; s <= H8; ++s) {
            Bitboard moves = EMPTY_BB;
            for (Direction d : Directions) {
                if (s + d <= H8 && s + d >= A1) {
                    moves |= SquareToBB(s + d);
                }
            }
            moves &= FileFromSquare(s) > FILE_D ? ~file_masks[FILE_A] : ~file_masks[FILE_H];
            king_moves[s] = moves;
        }
    }

    void InitKnightMoves() {
        int possible_moves[] = {6, 10, 15, 17, -6, -10, -15, -17};
        for (Square s = A1; s <= H8; ++s) {
            Bitboard moves = EMPTY_BB;
            for (int m : possible_moves) {
                if (s + m <= H8 && s + m >= A1) {
                    moves |= SquareToBB(s + m);
                }
            }
            moves &= FileFromSquare(s) > FILE_D ? ~file_masks[FILE_A] & ~file_masks[FILE_B] : ~file_masks[FILE_H] &
                                                                                              ~file_masks[FILE_G];
            knight_moves[s] = moves;
        }
    }
#pragma endregion


    // Init rays in all direction from every square to the edge of the board
    void InitRays() {
        for (Rank r = RANK_1; r < RANK_NR; ++r) {
            for (File f = FILE_A; f < FILE_NR; ++f) {
                Square s = SquareFromFiRa(f, r);

                Bitboard ray = ((rank_masks[r] ^ SquareToBB(s)) >> s) << s;
                rays[s][EAST_IDX] = ray;
                inner_rays[s][EAST_IDX] = ray & ~file_masks[FILE_H];

                ray = ((rank_masks[r] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                rays[s][WEST_IDX] = ray;
                inner_rays[s][WEST_IDX] = ray & ~file_masks[FILE_A];

                ray = ((file_masks[f] ^ SquareToBB(s)) >> s) << s;
                rays[s][NORTH_IDX] = ray;
                inner_rays[s][NORTH_IDX] = ray & ~rank_masks[RANK_8];

                ray = ((file_masks[f] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                rays[s][SOUTH_IDX] = ray;
                inner_rays[s][SOUTH_IDX] = ray & ~rank_masks[RANK_1];

                ray = EMPTY_BB;
                Bitboard files_to_move = SquareToBB(s);
                Bitboard attacked_square = files_to_move;
                do {
                    attacked_square >>= 9;
                    files_to_move >>= 1;
                    if (files_to_move & rank_masks[r]) {
                        ray |= attacked_square;
                    } else {
                        break;
                    }
                } while (attacked_square);
                rays[s][SOUTH_WEST_IDX] = ray;
                inner_rays[s][SOUTH_WEST_IDX] = ray & ~file_masks[FILE_A] & ~rank_masks[RANK_1];

                ray = EMPTY_BB;
                files_to_move = SquareToBB(s);
                attacked_square = files_to_move;
                do {
                    attacked_square >>= 7;
                    files_to_move <<= 1;
                    if (files_to_move & rank_masks[r]) {
                        ray |= attacked_square;
                    } else {
                        break;
                    }
                } while (attacked_square);
                rays[s][SOUTH_EAST_IDX] = ray;
                inner_rays[s][SOUTH_EAST_IDX] = ray & ~file_masks[FILE_H] & ~rank_masks[RANK_1];

                ray = EMPTY_BB;
                files_to_move = SquareToBB(s);
                attacked_square = files_to_move;
                do {
                    attacked_square <<= 9;
                    files_to_move <<= 1;
                    if (files_to_move & rank_masks[r]) {
                        ray |= attacked_square;
                    } else {
                        break;
                    }
                } while (attacked_square);
                rays[s][NORTH_EAST_IDX] = ray;
                inner_rays[s][NORTH_EAST_IDX] = ray & ~file_masks[FILE_H] & ~rank_masks[RANK_8];

                ray = EMPTY_BB;
                files_to_move = SquareToBB(s);
                attacked_square = files_to_move;
                do {
                    attacked_square <<= 7;
                    files_to_move >>= 1;
                    if (files_to_move & rank_masks[r]) {
                        ray |= attacked_square;
                    } else {
                        break;
                    }
                } while (attacked_square);
                rays[s][NORTH_WEST_IDX] = ray;
                inner_rays[s][NORTH_WEST_IDX] = ray & ~file_masks[FILE_A] & ~rank_masks[RANK_8];
            }
        }
    }

    Bitboard MakeRayToEdge(Square s1, Square s2) {

        File f1 = FileFromSquare(s1);
        Rank r1 = RankFromSquare(s1);
        File f2 = FileFromSquare(s2);
        Rank r2 = RankFromSquare(s2);

        Bitboard ray = EMPTY_BB;

        if (r1 == r2) {
            ray = rank_masks[r1];
        } else if (f1 == f2) {
            ray = file_masks[f1];
        } else if ((int) f1 + r1 == f2 + r2) {
            ray = diag_masks[f1 + r1];
        } else if ((int) f1 - r1 == (int) f2 - r2) {
            ray = anti_diag_masks[r1 + 7 - f1];
        }

        return ray;
    }


    Bitboard MakeRay(Square s1, Square s2) {

        Square min, max;

        if (s1 > s2) {
            max = s1;
            min = s2;
        } else {
            max = s2;
            min = s1;
        }

        Bitboard mask = SquareToBB(max) - (SquareToBB(min) << 1);

        Rank r_max = RankFromSquare(max);
        File f_max = FileFromSquare(max);

        Rank r_min = RankFromSquare(min);
        File f_min = FileFromSquare(min);

        Bitboard ray = EMPTY_BB;

        if (r_max == r_min) {
            ray = rank_masks[r_max] & mask;
        } else if (f_max == f_min) {
            ray = file_masks[f_max] & mask;
        } else if ((int) f_min + r_min == f_max + r_max) {
            ray = diag_masks[f_max + r_max] & mask;
        } else if ((int) f_min - r_min == (int) f_max - r_max) {
            ray = anti_diag_masks[r_max + 7 - f_max] & mask;
        }

        return ray;
    }

    void InitRaysBetweenSquares() {
        for (Square origin = A1; origin <= H8; ++origin) {
            for (Square destination = A1; destination <= H8; ++destination) {
                rays_between_squares[origin][destination] = MakeRay(origin, destination);
                rays_between_board_edges[origin][destination] = MakeRayToEdge(origin, destination);
            }
            rook_unbound_moves[origin] =
                    rays[origin][NORTH_IDX] | rays[origin][WEST_IDX] | rays[origin][EAST_IDX] | rays[origin][SOUTH_IDX];
            bishop_unbound_moves[origin] =
                    rays[origin][NORTH_WEST_IDX] | rays[origin][SOUTH_WEST_IDX] | rays[origin][SOUTH_EAST_IDX] |
                    rays[origin][NORTH_EAST_IDX];
        }
    }

    void Init() {
        InitRays();
        InitRaysBetweenSquares();
        InitMagic();
        InitKingMoves();
        InitKnightMoves();
        InitPawnAttacks();
    }

    Bitboard GetRookAttacks(Square s, Bitboard occ) {
        Magic m = rook_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    Bitboard GetBishopAttacks(Square s, Bitboard occ) {
        Magic m = bishop_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    template<PieceType PT>
    Bitboard GetAttacksForPiece(Square s, Bitboard occ, Color c) {
        switch (PT) {
            case PAWN:
                return pawn_attacks[c][s];
            case BISHOP :
                return GetBishopAttacks(s, occ);
            case ROOK :
                return GetRookAttacks(s, occ);
            case QUEEN :
                return GetBishopAttacks(s, occ) | GetRookAttacks(s, occ);
            case KNIGHT :
                return knight_moves[s];
            case KING :
                return king_moves[s];
        }
    }

    template Bitboard GetAttacksForPiece<PAWN>(Square, Bitboard, Color);
    template Bitboard GetAttacksForPiece<KNIGHT>(Square, Bitboard, Color);
    template Bitboard GetAttacksForPiece<BISHOP>(Square, Bitboard, Color);
    template Bitboard GetAttacksForPiece<ROOK>(Square, Bitboard, Color);
    template Bitboard GetAttacksForPiece<QUEEN>(Square, Bitboard, Color);
    template Bitboard GetAttacksForPiece<KING>(Square, Bitboard, Color);

    std::string PPBitboard(Bitboard b) {
        std::ostringstream oss;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            oss << std::to_string(r + 1) << " |";
            for (File f = FILE_A; f <= FILE_H; ++f) {
                if ((b >> ((r * 8) + f)) & 1) {
                    oss << " x ";
                } else {
                    oss << " o ";
                }
            }
            oss << "\n";
        }
        oss << "    A  B  C  D  E  F  G  H";
        return oss.str();
    }
}