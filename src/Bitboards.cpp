#include "Bitboards.h"
#include "Macros.h"

using namespace Meetra;

namespace Meetra {

    inline Bitboard RankMasks[RANK_NR]{
            0x00000000000000FFUL,
            0x000000000000FF00UL,
            0x0000000000FF0000UL,
            0x00000000FF000000UL,
            0x000000FF00000000UL,
            0x0000FF0000000000UL,
            0x00FF000000000000UL,
            0xFF00000000000000UL
    };

    inline Bitboard FileMasks[FILE_NR]{
            0x0101010101010101UL,
            0x0202020202020202UL,
            0x0404040404040404UL,
            0x0808080808080808UL,
            0x1010101010101010UL,
            0x2020202020202020UL,
            0x4040404040404040UL,
            0x8080808080808080UL
    };

    inline uint64_t RookMagicNum[SQUARE_NR] = {
            0xa8002c000108020ULL, 0x6c00049b0002001ULL, 0x100200010090040ULL, 0x2480041000800801ULL,
            0x280028004000800ULL,
            0x900410008040022ULL, 0x280020001001080ULL, 0x2880002041000080ULL, 0xa000800080400034ULL,
            0x4808020004000ULL,
            0x2290802004801000ULL, 0x411000d00100020ULL, 0x402800800040080ULL, 0xb000401004208ULL,
            0x2409000100040200ULL,
            0x1002100004082ULL, 0x22878001e24000ULL, 0x1090810021004010ULL, 0x801030040200012ULL, 0x500808008001000ULL,
            0xa08018014000880ULL, 0x8000808004000200ULL, 0x201008080010200ULL, 0x801020000441091ULL, 0x800080204005ULL,
            0x1040200040100048ULL, 0x120200402082ULL, 0xd14880480100080ULL, 0x12040280080080ULL, 0x100040080020080ULL,
            0x9020010080800200ULL, 0x813241200148449ULL, 0x491604001800080ULL, 0x100401000402001ULL,
            0x4820010021001040ULL,
            0x400402202000812ULL, 0x209009005000802ULL, 0x810800601800400ULL, 0x4301083214000150ULL,
            0x204026458e001401ULL,
            0x40204000808000ULL, 0x8001008040010020ULL, 0x8410820820420010ULL, 0x1003001000090020ULL,
            0x804040008008080ULL,
            0x12000810020004ULL, 0x1000100200040208ULL, 0x430000a044020001ULL, 0x280009023410300ULL,
            0xe0100040002240ULL,
            0x200100401700ULL, 0x2244100408008080ULL, 0x8000400801980ULL, 0x2000810040200ULL, 0x8010100228810400ULL,
            0x2000009044210200ULL, 0x4080008040102101ULL, 0x40002080411d01ULL, 0x2005524060000901ULL,
            0x502001008400422ULL,
            0x489a000810200402ULL, 0x1004400080a13ULL, 0x4000011008020084ULL, 0x26002114058042ULL
    };


    void InitRookMasks() {
        for (Rank r = RANK_1; r <= RANK_8; ++r) {
            for (File f = FILE_A; f <= FILE_H; ++f) {
                Square s = SquareFromFiRa(f, r);
                RookMasks[s] = Rays[s][NORTH_IDX] | Rays[s][EAST_IDX] | Rays[s][SOUTH_IDX] | Rays[s][WEST_IDX];
            }
        }

        //std::cout << "Mask: " << RookMasks[B3] << std::endl;
    }

    void InitBishopMasks(){
        for (Rank r = RANK_1; r <= RANK_8; ++r) {
            for (File f = FILE_A; f <= FILE_H; ++f) {
                Square s = SquareFromFiRa(f, r);
                BishopMasks[s] = Rays[s][NORTH_WEST_IDX] | Rays[s][NORTH_EAST_IDX] |
                        Rays[s][SOUTH_WEST_IDX] | Rays[s][SOUTH_EAST_IDX] ;
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

    //https://github.com/GunshipPenguin/shallow-blue/blob/c6d7e9615514a86533a9e0ffddfc96e058fc9cfd/src/attacks.cc
    //https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html - good, source above ^

    //http://pradu.us/old/Nov27_2008/Buzz/research/magic/Bitboards.pdf

    //https://essays.jwatzman.org/essays/chess-move-generation-with-magic-bitboards.html - essay on magics

    // https://www.chessprogramming.org/Looking_for_Magics#cite_note-2 - generator code

    inline void InitRays() {
        for (Rank r = RANK_1; r < RANK_NR; ++r) {
            for (File f = FILE_A; f < FILE_NR; ++f) {
                Square s = SquareFromFiRa(f, r);

                Bitboard ray = ((RankMasks[r] ^ SquareToBB(s)) >> s) << s;
                Rays[s][EAST_IDX] = ray;
                ray = ((RankMasks[r] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                Rays[s][WEST_IDX] = ray;

                ray = ((FileMasks[f] ^ SquareToBB(s)) >> s) << s;
                Rays[s][NORTH_IDX] = ray;
                ray = ((FileMasks[f] ^ SquareToBB(s)) << (SQUARE_NR - (s + 1))) >> (SQUARE_NR - (s + 1));
                Rays[s][SOUTH_IDX] = ray;

                ray = 0UL;
                Bitboard attack_square = SquareToBB(s);
                Bitboard moved_files = attack_square;
                do {
                    attack_square >>= 9;
                    moved_files >>= 1;
                    if (moved_files & RankMasks[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                Rays[s][SOUTH_WEST_IDX] = ray;

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square >>= 7;
                    moved_files <<= 1;
                    if (moved_files & RankMasks[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                Rays[s][SOUTH_EAST_IDX] = ray;

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square <<= 9;
                    moved_files <<= 1;
                    if (moved_files & RankMasks[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                Rays[s][NORTH_EAST_IDX] = ray;

                ray = 0UL;
                attack_square = SquareToBB(s);
                moved_files = attack_square;
                do {
                    attack_square <<= 7;
                    moved_files >>= 1;
                    if (moved_files & RankMasks[r]) {
                        ray |= attack_square;
                    } else {
                        break;
                    }
                } while (attack_square);
                Rays[s][NORTH_WEST_IDX] = ray;
            }
        }

        //std::cout << PPBitboard(Rays[H1][NORTH_WEST_IDX]) << std::endl;
    }

    void InitBitboards() {
        // init the directions used for init masks for attacks
        InitRays();

        // init the masks for attacks
        InitBishopMasks();
        InitRookMasks();

        // init magic numbers (instead of hard-coded)
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