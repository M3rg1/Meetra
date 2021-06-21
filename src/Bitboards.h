#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Types.h"
#include <bit>
#include <string>
#include <iostream>

namespace Meetra {


/*    inline uint64_t BishopMagicNum[SQUARE_NR] = {
            0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL, 0x62880a0220200808ULL, 0x4042004000000ULL,
            0x100822020200011ULL, 0xc00444222012000aULL, 0x28808801216001ULL, 0x400492088408100ULL, 0x201c401040c0084ULL,
            0x840800910a0010ULL, 0x82080240060ULL, 0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL,
            0x8144042209100900ULL, 0x208081020014400ULL, 0x4800201208ca00ULL, 0xf18140408012008ULL, 0x1004002802102001ULL,
            0x841000820080811ULL, 0x40200200a42008ULL, 0x800054042000ULL, 0x88010400410c9000ULL, 0x520040470104290ULL,
            0x1004040051500081ULL, 0x2002081833080021ULL, 0x400c00c010142ULL, 0x941408200c002000ULL, 0x658810000806011ULL,
            0x188071040440a00ULL, 0x4800404002011c00ULL, 0x104442040404200ULL, 0x511080202091021ULL, 0x4022401120400ULL,
            0x80c0040400080120ULL, 0x8040010040820802ULL, 0x480810700020090ULL, 0x102008e00040242ULL, 0x809005202050100ULL,
            0x8002024220104080ULL, 0x431008804142000ULL, 0x19001802081400ULL, 0x200014208040080ULL, 0x3308082008200100ULL,
            0x41010500040c020ULL, 0x4012020c04210308ULL, 0x208220a202004080ULL, 0x111040120082000ULL, 0x6803040141280a00ULL,
            0x2101004202410000ULL, 0x8200000041108022ULL, 0x21082088000ULL, 0x2410204010040ULL, 0x40100400809000ULL,
            0x822088220820214ULL, 0x40808090012004ULL, 0x910224040218c9ULL, 0x402814422015008ULL, 0x90014004842410ULL,
            0x1000042304105ULL, 0x10008830412a00ULL, 0x2520081090008908ULL, 0x40102000a0a60140ULL,
    };*/

    inline Bitboard RookAttackTable[64][4096];
    //inline Bitboard BishopAttackTable[64][1024];

    inline Bitboard RookMasks[64];
    inline Bitboard BishopMasks[64];
    //inline Bitboard BishopMasks[64];

    inline Bitboard Rays[SQUARE_NR][DIRECTION_IDX_NR];

    void InitBitboards();


/*    Bitboard GetRookAttacks(Square s, Bitboard blockers) {
        // Mask blockers to only include bits on diagonals
        blockers &= RookMasks[s];

        // Generate the key using a multiplication and right shift
        uint64_t key = (blockers * RookMagicNum[s]) >> (64 - BISHOP_INDEX_BITS[s]);

        // Return the preinitialized attack set bitboard from the table
        return RookAttackTable[s][key];
    }*/


    std::string PPBitboard(Bitboard b);

    // linux builtins
    inline constexpr Square Lsb(Bitboard b) {
        return Square(__builtin_ctzll(b));
    }

    inline constexpr Square Msb(Bitboard b) {
        return Square(63 ^ __builtin_clzll(b));
    }

    inline constexpr Square PopLsb(Bitboard &b) {
        const Square s = Lsb(b);
        b &= b - 1;
        return s;
    }

    inline constexpr int PopCount(Bitboard b) {
        return std::__popcount(b);
    }

    inline constexpr Bitboard SquareToBB(Square s){
        return 1UL << s;
    }


    //squareIndex = 8*rankIndex + fileIndex
    //rankIndex   = squareIndex div 8
    //fileIndex   = squareIndex mod 8



    // inline Bitboard SquareToBitboard(Square s){} ..
}

// win64 https://www.chessprogramming.org/BitScan -> Processor Instructions for Bitscans
/*inline Square Lsb(Bitboard b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square) idx;
}

inline Square Msb(Bitboard b) {
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square) idx;
}*/



#endif //MEETRA_BITBOARDS_H
