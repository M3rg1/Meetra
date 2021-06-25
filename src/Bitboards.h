#ifndef MEETRA_BITBOARDS_H
#define MEETRA_BITBOARDS_H

#include "Types.h"
#include <bit>
#include <string>
#include <iostream>

namespace Meetra {

    void InitBitboards();

    struct Magic {
        Bitboard *attacks; // pointer into the rook/bishop table, where all the attacks are stored
        Bitboard inner_board_mask;
        uint64_t magic_num;
        uint8_t shift;
    };

    extern Bitboard rank_masks[RANK_NR];
    extern Bitboard file_masks[FILE_NR];

    extern Magic bishop_magics[SQUARE_NR];
    extern Magic rook_magics[SQUARE_NR];

    extern Bitboard king_moves[SQUARE_NR];
    extern Bitboard knight_moves[SQUARE_NR];

    extern Bitboard pawn_attacks[COLOR_NR][SQUARE_NR];

    extern Bitboard rays_between_squares[SQUARE_NR][SQUARE_NR];

    inline Bitboard GetRookAttacks(Square s, Bitboard occ) {
        Magic m = rook_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    inline Bitboard GetBishopAttacks(Square s, Bitboard occ) {
        Magic m = bishop_magics[s];
        return m.attacks[((occ & m.inner_board_mask) * m.magic_num) >> m.shift];
    }

    inline Bitboard SquareToBB(Square s) { return static_cast<Bitboard>(0x1) << s; }
    inline void SetBBSquareOne(Bitboard &b, Square s) { b |= SquareToBB(s); }
    inline void SetBBSquareZero(Bitboard &b, Square s) { b &= ~SquareToBB(s); }

    //https://www.youtube.com/watch?v=JJ_OJFM-z5E
    // 4:25
    // testing if a square is under attack TODO

/*    template<PieceType pt>
    inline Bitboard GetAttacksForPiece(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE) {
        //occ &= ~SquareToBB(s);
        switch (pt) {
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
    }*/

    template<PieceType pt>
    inline Bitboard GetAttacksForPiece(Square s, Bitboard occ = EMPTY_BB, Color c = WHITE) {
        if (pt == PAWN) {
            return pawn_attacks[c][s];
        } else if(pt == KNIGHT) {
            return knight_moves[s];
        } else if(pt == BISHOP) {
            return GetBishopAttacks(s, occ);
        } else if(pt == ROOK) {
            return GetRookAttacks(s, occ);
        } else if(pt == QUEEN) {
            return GetBishopAttacks(s, occ) | GetRookAttacks(s, occ);
        } else {
            return king_moves[s];
        }
    }

    template<Color c>
    inline Bitboard GetPawnOneForward(Bitboard pawns, Bitboard empty_squares) {
        switch (c) {
            case WHITE:
                return (pawns << 8) & empty_squares;
            case BLACK:
                return (pawns >> 8) & empty_squares;
        }
    }

    template<Color c>
    inline Bitboard GetPawnTwoForward(Bitboard pawns, Bitboard empty_squares) {
        switch (c) {
            case WHITE:
                pawns &= rank_masks[RANK_2];
                break;
            case BLACK:
                pawns &= rank_masks[RANK_7];
                break;
        }
        return GetPawnOneForward<c>(GetPawnOneForward<c>(pawns, empty_squares), empty_squares);
    }

    std::string PPBitboard(Bitboard b);

    inline int PopCount(Bitboard b) { return std::__popcount(b); }
    //linux builtins
    inline Square Lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }

    inline Square PopLsb(Bitboard &b) {
        const Square s = Lsb(b);
        b &= b - 1;
        return s;
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

}

#endif //MEETRA_BITBOARDS_H
