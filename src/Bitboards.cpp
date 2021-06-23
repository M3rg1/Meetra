#include "Bitboards.h"
#include "MagicNumbers.h"

namespace Meetra {

    Bitboard rank_masks[RANK_NR]{
            0x00000000000000FFUL,
            0x000000000000FF00UL,
            0x0000000000FF0000UL,
            0x00000000FF000000UL,
            0x000000FF00000000UL,
            0x0000FF0000000000UL,
            0x00FF000000000000UL,
            0xFF00000000000000UL
    };

    Bitboard file_masks[FILE_NR]{
            0x0101010101010101UL,
            0x0202020202020202UL,
            0x0404040404040404UL,
            0x0808080808080808UL,
            0x1010101010101010UL,
            0x2020202020202020UL,
            0x4040404040404040UL,
            0x8080808080808080UL
    };

    Magic bishop_magics[64];
    Magic rook_magics[64];

    Bitboard king_moves[64];
    Bitboard knight_moves[64];

    Bitboard rays[SQUARE_NR][DIRECTION_IDX_NR];
    Bitboard inner_rays[SQUARE_NR][DIRECTION_IDX_NR];

    Bitboard rook_table[102400];
    Bitboard bishop_table[5248];

#pragma region ===== Hyperbola Quintessence, Reverse Bitboards (for magics initialization) =====

    constexpr Bitboard ReverseBits(Bitboard b) {
        b = ((b >> 1) & 0x5555555555555555UL) | ((b & 0x5555555555555555UL) << 1);
        b = ((b >> 2) & 0x3333333333333333UL) | ((b & 0x3333333333333333UL) << 2);
        b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FUL) | ((b & 0x0F0F0F0F0F0F0F0FUL) << 4);
        b = ((b >> 8) & 0x00FF00FF00FF00FFUL) | ((b & 0x00FF00FF00FF00FFUL) << 8);
        b = ((b >> 16) & 0x0000FFFF0000FFFFUL) | ((b & 0x0000FFFF0000FFFFUL) << 16);
        b = (b >> 32) | (b << 32);
        return b;
    }

    Bitboard GetDiagMoves(Square s, Bitboard occ) {
        ulong bitboard = SquareToBB(s);
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

        SetBBSquareOne(blockers, current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);

        SetBBSquareZero(blockers, current);
        SetBlockersRecursive(m, origin, blockers, explore_occ, move_generator);

    }

    void FillMagics() {
        for (Square s = A1; s <= H8; ++s) {
            SetBlockersRecursive(rook_magics[s], s, EMPTY_BB,
                                 rays[s][NORTH_IDX] | rays[s][EAST_IDX] | rays[s][SOUTH_IDX] | rays[s][WEST_IDX],
                                 GetHorAndVertMoves);
            SetBlockersRecursive(bishop_magics[s], s, EMPTY_BB,
                                 rays[s][NORTH_WEST_IDX] | rays[s][NORTH_EAST_IDX] | rays[s][SOUTH_WEST_IDX] |
                                 rays[s][SOUTH_EAST_IDX], GetDiagAndAntiDiagMoves);
        }
    }

    void InitMagic() {
        for (Square s = A1; s <= H8; ++s) {
            rook_magics[s].shift = 64 - rook_magic_shift[s];
            rook_magics[s].inner_board_mask = inner_rays[s][NORTH_IDX] | inner_rays[s][EAST_IDX]
                                              | inner_rays[s][SOUTH_IDX] | inner_rays[s][WEST_IDX];
            rook_magics[s].magic_num = rook_magic_num[s];
            rook_magics[s].attacks = s == A1 ? rook_table : rook_magics[s - 1].attacks + (1 << rook_magic_shift[s]);

            bishop_magics[s].shift = 64 - bishop_magic_shift[s];
            bishop_magics[s].inner_board_mask = inner_rays[s][NORTH_WEST_IDX] | inner_rays[s][NORTH_EAST_IDX]
                                                | inner_rays[s][SOUTH_EAST_IDX] | inner_rays[s][SOUTH_WEST_IDX];
            bishop_magics[s].magic_num = bishop_magic_num[s];
            bishop_magics[s].attacks =
                    s == A1 ? bishop_table : bishop_magics[s - 1].attacks + (1 << bishop_magic_shift[s]);
        }

        FillMagics();
    }
#pragma endregion


#pragma region ===== Precomputing king and knight moves =====

    void InitKingMoves() {
        for (Square s = A1; s <= H8; ++s) {
            Bitboard moves = EMPTY_BB;
            for (Direction d : Directions) {
                if (s + d <= H8 && s + d >= A1) {
                    SetBBSquareOne(moves, s + d);
                }
            }
            moves &= (s & 7) > 3 ? ~file_masks[FILE_A] : ~file_masks[FILE_H];
            king_moves[s] = moves;
        }
    }

    void InitKnightMoves() {
        int possible_moves[] = {6, 10, 15, 17, -6, -10, -15, -17};
        for (Square s = A1; s <= H8; ++s) {
            Bitboard moves = EMPTY_BB;
            for (int m : possible_moves) {
                if (s + m <= H8 && s + m >= A1) {
                    SetBBSquareOne(moves, s + m);
                }
            }
            moves &=
                    (s & 7) > 3 ? ~file_masks[FILE_A] & ~file_masks[FILE_B] : ~file_masks[FILE_H] & ~file_masks[FILE_G];
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

    void InitBitboards() {
        InitRays();
        InitMagic();
        InitKingMoves();
        InitKnightMoves();
    }

    std::string PPBitboard(Bitboard b) {
        std::string ret;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            ret.append(std::to_string(r + 1));
            ret.append(" |");
            for (File f = FILE_A; f <= FILE_H; ++f) {
                if ((b >> (r * 8) + f) & 1) {
                    ret.append(" x ");
                } else {
                    ret.append(" o ");
                }
            }
            ret.append("\n");
        }
        ret.append("    A  B  C  D  E  F  G  H");
        return ret;
    }
}