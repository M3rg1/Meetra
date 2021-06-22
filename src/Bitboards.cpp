#include "Bitboards.h"
#include "Macros.h"

using namespace Meetra;

namespace Meetra {

    struct Magic {
        Bitboard *attacks; // how many possible blocker combinations can be made on this square
        Bitboard inner_board_mask;
        uint64_t magic_num;
        uint16_t shift;
    };

    Magic bishop_magics[64];
    Magic rook_magics[64];

    inline Bitboard GetRookAttacks(Square s, Bitboard occ) {
        Magic m = rook_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    inline Bitboard GetBishopAttacks(Square s, Bitboard occ) {
        Magic m = bishop_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    Bitboard rays[SQUARE_NR][DIRECTION_IDX_NR];
    Bitboard inner_rays[SQUARE_NR][DIRECTION_IDX_NR];

    Bitboard rook_table[102400];
    Bitboard bishop_table[5248];

    // just combined directional rays into one mask
    Bitboard rook_move_mask[64];
    Bitboard bishop_move_mask[64];

    // i should just generate these in code instead of hard coding tbh TODO ... mabybe
    constexpr Bitboard rank_mask[RANK_NR]{
            0x00000000000000FFUL,
            0x000000000000FF00UL,
            0x0000000000FF0000UL,
            0x00000000FF000000UL,
            0x000000FF00000000UL,
            0x0000FF0000000000UL,
            0x00FF000000000000UL,
            0xFF00000000000000UL
    };

    constexpr Bitboard file_mask[FILE_NR]{
            0x0101010101010101UL,
            0x0202020202020202UL,
            0x0404040404040404UL,
            0x0808080808080808UL,
            0x1010101010101010UL,
            0x2020202020202020UL,
            0x4040404040404040UL,
            0x8080808080808080UL
    };

    constexpr uint64_t bishop_magic_num[SQUARE_NR] = {
            0xc085080200420200,
            0x60014902028010,
            0x401240100c201,
            0x580ca104020080,
            0x8434052000230010,
            0x102080208820420,
            0x2188410410403024,
            0x40120805282800,
            0x4420410888208083,
            0x1049494040560,
            0x6090100400842200,
            0x1000090405002001,
            0x48044030808c409,
            0x20802080384,
            0x2012008401084008,
            0x9741088200826030,
            0x822000400204c100,
            0x14806004248220,
            0x30200101020090,
            0x148150082004004,
            0x6020402112104,
            0x4001000290080d22,
            0x2029100900400,
            0x804203145080880,
            0x60a10048020440,
            0xc08080b20028081,
            0x1009001420c0410,
            0x101004004040002,
            0x1004405014000,
            0x10029a0021005200,
            0x4002308000480800,
            0x301025015004800,
            0x2402304004108200,
            0x480110c802220800,
            0x2004482801300741,
            0x400400820a60200,
            0x410040040040,
            0x2828080020011000,
            0x4008020050040110,
            0x8202022026220089,
            0x204092050200808,
            0x404010802400812,
            0x422002088009040,
            0x180604202002020,
            0x400109008200,
            0x2420042000104,
            0x40902089c008208,
            0x4001021400420100,
            0x484410082009,
            0x2002051108125200,
            0x22e4044108050,
            0x800020880042,
            0xb2020010021204a4,
            0x2442100200802d,
            0x10100401c4040000,
            0x2004a48200c828,
            0x9090082014000,
            0x800008088011040,
            0x4000000a0900b808,
            0x900420000420208,
            0x4040104104,
            0x120208c190820080,
            0x4000102042040840,
            0x8002421001010100,
    };

    constexpr uint64_t rook_magic_num[SQUARE_NR] = {
            0x11800040001481a0,
            0x2040400010002000,
            0xa280200308801000,
            0x100082005021000,
            0x280280080040006,
            0x200080104100200,
            0xc00040221100088,
            0xe00072200408c01,
            0x2002045008600,
            0xa410804000200089,
            0x4081002000401102,
            0x2000c20420010,
            0x800800400080080,
            0x40060010041a0009,
            0x441004442000100,
            0x462800080004900,
            0x80004020004001,
            0x1840420021021081,
            0x8020004010004800,
            0x940220008420010,
            0x2210808008000400,
            0x24808002000400,
            0x803604001019a802,
            0x520000440081,
            0x802080004000,
            0x1200810500400024,
            0x8000100080802000,
            0x2008080080100480,
            0x8000404002040,
            0xc012040801104020,
            0xc015000900240200,
            0x20040200208041,
            0x1080004000802080,
            0x400081002110,
            0x30002000808010,
            0x2000100080800800,
            0x2c0800400800800,
            0x1004800400800200,
            0x818804000210,
            0x340082000a45,
            0x8520400020818000,
            0x2008900460020,
            0x100020008080,
            0x601001000a30009,
            0xc001000408010010,
            0x2040002008080,
            0x11008218018c0030,
            0x20c0080620011,
            0x400080002080,
            0x8810040002500,
            0x400801000200080,
            0x2402801000080480,
            0x204040280080080,
            0x31044090200801,
            0x40c10830020400,
            0x442800100004080,
            0x10080002d005041,
            0x134302820010a2c2,
            0x6202001080200842,
            0x1820041000210009,
            0x1002001008210402,
            0x2000108100402,
            0x10310090a00b824,
            0x800040100944822,
    };

    constexpr int rook_magic_shift[64] = {
            12, 11, 11, 11, 11, 11, 11, 12,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            12, 11, 11, 11, 11, 11, 11, 12,
    };

    constexpr int bishop_magic_shift[64] = {
            6, 5, 5, 5, 5, 5, 5, 6,
            5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 7, 7, 7, 7, 5, 5,
            5, 5, 7, 9, 9, 7, 5, 5,
            5, 5, 7, 9, 9, 7, 5, 5,
            5, 5, 7, 7, 7, 7, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5,
            6, 5, 5, 5, 5, 5, 5, 6,
    };

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

    Bitboard GenerateBishopMoves(Square s, Bitboard occ) {
        return GetAntiDiagMoves(s, occ) | GetDiagMoves(s, occ);
    }

    Bitboard GetVerticalMoves(Square s, Bitboard occ) {
        Bitboard b = SquareToBB(s);
        File f = FileFromSquare(s);
        return (((occ & file_mask[f]) - (b << 1)) ^
                ReverseBits(ReverseBits(occ & file_mask[f]) - (ReverseBits(b) << 1))) & file_mask[f];
    }

    Bitboard GetHorizontalMoves(Square s, Bitboard occ) {
        Bitboard b = SquareToBB(s);
        Rank r = RankFromSquare(s);
        return ((occ - (b << 1)) ^ ReverseBits(ReverseBits(occ) - (ReverseBits(b) << 1))) & rank_mask[r];
    }

    Bitboard GenerateRookMoves(Square s, Bitboard occ) {
        return GetVerticalMoves(s, occ) | GetHorizontalMoves(s, occ);
    }

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
            SetBlockersRecursive(rook_magics[s], s, 0UL, rook_move_mask[s], GenerateRookMoves);
            SetBlockersRecursive(bishop_magics[s], s, 0UL, bishop_move_mask[s], GenerateBishopMoves);
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

    void InitRookMasks() {
        for (Rank r = RANK_1; r <= RANK_8; ++r) {
            for (File f = FILE_A; f <= FILE_H; ++f) {
                Square s = SquareFromFiRa(f, r);
                rook_move_mask[s] = rays[s][NORTH_IDX] | rays[s][EAST_IDX] | rays[s][SOUTH_IDX] | rays[s][WEST_IDX];
            }
        }
    }

    void InitBishopMasks() {
        for (Rank r = RANK_1; r <= RANK_8; ++r) {
            for (File f = FILE_A; f <= FILE_H; ++f) {
                Square s = SquareFromFiRa(f, r);
                bishop_move_mask[s] = rays[s][NORTH_WEST_IDX] | rays[s][NORTH_EAST_IDX] | rays[s][SOUTH_WEST_IDX] |
                                      rays[s][SOUTH_EAST_IDX];
            }
        }
    }

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// pro vytvareni toho klice nezalezi na okupaci hran, protoze
// at uz je hrana okupovana nebo ne, tak vrati stejny attack square map
// ten tutorial na webu je debilni, attack mapy potrebuju vsechny,
// jen ten klic s occupied edge nebo neoccupied edge by racel stejnou mapu
// takze na nej se muzu vykaslat

    void InitRays() {
        for (Rank r = RANK_1; r < RANK_NR; ++r) {
            for (File f = FILE_A; f < FILE_NR; ++f) {
                Square s = SquareFromFiRa(f, r);

                Bitboard ray = ((rank_mask[r] ^ SquareToBB(s)) >> s) << s;
                rays[s][EAST_IDX] = ray;
                inner_rays[s][EAST_IDX] = ray & ~file_mask[FILE_H];

                ray = ((rank_mask[r] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                rays[s][WEST_IDX] = ray;
                inner_rays[s][WEST_IDX] = ray & ~file_mask[FILE_A];

                ray = ((file_mask[f] ^ SquareToBB(s)) >> s) << s;
                rays[s][NORTH_IDX] = ray;
                inner_rays[s][NORTH_IDX] = ray & ~rank_mask[RANK_8];

                ray = ((file_mask[f] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                rays[s][SOUTH_IDX] = ray;
                inner_rays[s][SOUTH_IDX] = ray & ~rank_mask[RANK_1];

                ray = 0UL;
                Bitboard attack_square = SquareToBB(s);
                Bitboard moved_files = attack_square;
                do {
                    attack_square >>= 9;
                    moved_files >>= 1;
                    if (moved_files & rank_mask[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                rays[s][SOUTH_WEST_IDX] = ray;
                inner_rays[s][SOUTH_WEST_IDX] = ray & ~file_mask[FILE_A] & ~rank_mask[RANK_1];

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square >>= 7;
                    moved_files <<= 1;
                    if (moved_files & rank_mask[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                rays[s][SOUTH_EAST_IDX] = ray;
                inner_rays[s][SOUTH_EAST_IDX] = ray & ~file_mask[FILE_H] & ~rank_mask[RANK_1];

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square <<= 9;
                    moved_files <<= 1;
                    if (moved_files & rank_mask[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                rays[s][NORTH_EAST_IDX] = ray;
                inner_rays[s][NORTH_EAST_IDX] = ray & ~file_mask[FILE_H] & ~rank_mask[RANK_8];

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square <<= 7;
                    moved_files >>= 1;
                    if (moved_files & rank_mask[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                rays[s][NORTH_WEST_IDX] = ray;
                inner_rays[s][NORTH_WEST_IDX] = ray & ~file_mask[FILE_A] & ~rank_mask[RANK_8];
            }
        }
    }

    void InitBitboards() {
        // init the directions used for init masks for attacks
        InitRays();

        // init the masks for attacks
        InitBishopMasks();
        InitRookMasks();

        InitMagic();

        // init magic_num numbers (instead of hard-coded)
        //InitRookMagics();
        //InitBishopMagics();

    }

    std::string PPBitboard(Bitboard b) {
        std::string ret;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            ret.append(std::to_string(r + 1));
            ret.append(" |");
            for (File f = FILE_A; f <= FILE_H; ++f) {
                //DEBUG_LOG(board[SquareFromFiRa(f, r)]);
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