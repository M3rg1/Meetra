#ifndef MEETRA_DEFS_H
#define MEETRA_DEFS_H

#include <string>

namespace Meetra {

    using TimeRep = int64_t;

    using Depth = int;

    using Score = int;

    using Hash64 = uint64_t;
    using Hash16 = uint16_t;
    constexpr Hash64 NEW_HASH64 = 0;

    using Bitboard = uint64_t;
    constexpr Bitboard EMPTY_BB = 0;

    enum Colors {
        WHITE, BLACK,
        COLOR_NR
    };
    using Color = int;

    constexpr Color OtherColor(Color c) { return c ^ 1; }

    enum PieceTypes {
        NONE_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
        PIECE_TYPE_NR,
        ALL_TYPES = 7
    };
    using PieceType = int;

    enum Pieces {
        NO_PIECE = 0,
        W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
        B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    };
    using Piece = int;

    constexpr Color ColorOfPiece(Piece p) { return p >> 3; }
    constexpr PieceType TypeOfPiece(Piece p) { return p & 7; }
    constexpr Piece NewPiece(PieceType pt, Color c) { return (c << 3) | pt; }

    constexpr std::string_view piece_char = "oPNBRQK  pnbrqk";

    constexpr Piece CharToPiece(char c) { return static_cast<Piece>(piece_char.find(c)); }
    constexpr char PieceToChar(Piece p) { return piece_char[p]; }

    enum Ranks {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
        RANK_NR
    };
    using Rank = int;

    enum Files {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
        FILE_NR
    };
    using File = int;

    constexpr Rank RankFromChar(char c) { return c - '1'; }
    constexpr char CharFromRank(Rank r) { return static_cast<char>(r + '1'); }
    constexpr File FileFromChar(char c) { return c - 'a'; }
    constexpr char CharFromFile(File f) { return static_cast<char>(f + 'a'); }

    enum PawnMoveDir {
        LEFT, RIGHT, FORWARD,
    };

    enum Directions {
        NORTH = 8, NORTH_EAST = 9, EAST = 1, SOUTH_EAST = -7, SOUTH = -8, SOUTH_WEST = -9, WEST = -1, NORTH_WEST = 7
    };
    using Direction = int;

    enum Squares {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
        SQUARE_NR,
        NO_SQ = 0
    };
    using Square = int;

    constexpr Bitboard SquareToBB(Square s) { return 0x1ULL << s; }
    constexpr Square SqFromFiRa(File f, Rank r) { return (r << 3) | f; }
    constexpr File FileFromSquare(Square s) { return s & 7; }
    constexpr Rank RankFromSquare(Square s) { return s >> 3; }
    constexpr Square NameToSquare(std::string_view name) {
        return SqFromFiRa(FileFromChar(name[0]), RankFromChar(name[1]));
    }
    inline std::string SquareToName(Square s) {
        return {CharFromFile(FileFromSquare(s)), CharFromRank(RankFromSquare(s))};
    }

    enum CastlingSide {
        SHORT, LONG, CS_NR
    };

    /**
     * Move bits:
     * 0-5 from square
     * 6-11 to square
     * 12-15 MoveType flag
     * if the 14th bit is 1, it's a promotion move -> prom bits  N = 0010.., B = 1010.., R = 0110.., Q = 1110..
     */
    using Move = uint16_t;
    constexpr Move ZERO_MOVE = 0;

    enum MoveTypes {
        NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 12, PROMOTE_BISHOP = 5 << 12, PROMOTE_ROOK = 6 << 12, PROMOTE_QUEEN = 7 << 12
    };
    using MoveType = int;

    constexpr Move NewMove(Square from, Square to) { return from | (to << 6); }
    constexpr Move NewMove(Square from, Square to, MoveType flag) { return NewMove(from, to) | flag; }

    constexpr PieceType PieceTypeFromFlag(MoveType mt) { return (mt >> 12) - 2; }
    constexpr Square FromSquare(Move m) { return m & 0x3F; }
    constexpr Square ToSquare(Move m) { return (m >> 6) & 0x3F; }
    constexpr bool IsPromotion(Move m) { return m >> 14; }
    constexpr MoveType GetMoveType(Move m) { return m & 0xF000; }
}

#endif //MEETRA_DEFS_H
