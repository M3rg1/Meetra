#ifndef MEETRA_TYPES_H
#define MEETRA_TYPES_H

#include <string>

namespace Meetra {

    using Depth = int;

    using Score = int;

    using Hash64 = uint64_t;
    using Hash32 = uint32_t;
#define NEW_HASH 0

    using Bitboard = uint64_t;
#define EMPTY_BB 0

    enum Color {
        WHITE, BLACK,
        COLOR_NR
    };

    inline Color OtherColor(Color c) {
        return static_cast<Color>(c ^ 1);
    }

    enum PieceType {
        NONE_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6,
        PIECE_TYPE_NR,
        ALL_TYPES = 7
    };

    enum Piece  {
        NO_PIECE = 0,
        W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
        B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    };

    inline Color ColorOfPiece(Piece p) { return static_cast<Color>(p >> 3); }
    inline PieceType TypeOfPiece(Piece p) { return static_cast<PieceType>(p & 7); }
    inline Piece NewPiece(PieceType pt, Color c) { return static_cast<Piece>((c << 3) | pt); }

    enum Rank {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
        RANK_NR
    };

    enum File {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
        FILE_NR
    };

    inline Rank RankFromChar(char c) { return static_cast<Rank>(c - '1'); }
    inline char CharFromRank(Rank r) { return static_cast<char>(r + '1'); }
    inline File FileFromChar(char c) { return static_cast<File>(c - 'a'); }
    inline char CharFromFile(File f) { return static_cast<char>(f + 'a'); }

    inline std::string_view piece_char = "oPNBRQK  pnbrqk";

    inline Piece CharToPiece(char c) {
        return static_cast<Piece>(piece_char.find(c));
    }

    inline char PieceToChar(Piece p) {
        return piece_char[p];
    }

    enum PawnMoveDir {
        LEFT, RIGHT, FORWARD,
    };

    enum Direction {
        NORTH = 8, NORTH_EAST = 9, EAST = 1, SOUTH_EAST = -7, SOUTH = -8, SOUTH_WEST = -9, WEST = -1, NORTH_WEST = 7
    };

    enum Square {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
        SQUARE_NR,
        ZERO_SQ = 0
    };

    inline Bitboard SquareToBB(Square s) { return static_cast<Bitboard>(0x1) << s; }
    inline Square SqFromFiRa(File f, Rank r) { return static_cast<Square>((r << 3) | f); }
    inline File FileFromSquare(Square s) { return static_cast<File>(s & 7); }
    inline Rank RankFromSquare(Square s) { return static_cast<Rank>(s >> 3); }
    inline Square NameToSquare(const std::string& name) {
        return SqFromFiRa(FileFromChar(name[0]), RankFromChar(name[1]));
    }
    inline std::string SquareToName(Square s) {
        return {CharFromFile(FileFromSquare(s)), CharFromRank(RankFromSquare(s))};
    }

    enum CastlingSide {
        SHORT, LONG, CS_NR
    };

#pragma region ===== Move =====
    /**
     * Move bits:
     * 0-5 from square
     * 6-11 to square
     * 12-15 MoveType flag
     * if the 14th bit is 1, it's a promotion move -> prom bits  N = 0010, B = 1010, R = 0110, Q = 0111
     */
    using Move = uint16_t;

    enum MoveType {
        ZERO_MOVE = 0, NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 12, PROMOTE_BISHOP = 5 << 12, PROMOTE_ROOK = 6 << 12, PROMOTE_QUEEN = 7 << 12
    };

    inline Move NewMove(Square from, Square to) { return static_cast<Move>(from | to << 6); }
    inline Move NewMove(Square from, Square to, MoveType flag) { return static_cast<Move>(NewMove(from, to) | flag); }

    inline PieceType PieceTypeFromFlag(MoveType mt) { return static_cast<PieceType>((mt >> 12) - 2); }
    inline Square FromSquare(Move m) { return static_cast<Square>(m & 0x3F); }
    inline Square ToSquare(Move m) { return static_cast<Square>(m >> 6 & 0x3F); }
    inline bool IsPromotion(Move m) { return m >> 14; }
    inline MoveType GetMoveType(Move m) { return static_cast<MoveType>(m & 0xF000); }

#pragma endregion


#pragma region ===== Operator overloading settings =====

#define ENABLE_BASE_OPERATORS_ON(T)                                \
inline T operator+(T d1, int d2) { return T(int(d1) + d2); }    \
inline T operator-(T d1, int d2) { return T(int(d1) - d2); }    \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }       \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }      \

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

    ENABLE_INCR_OPERATORS_ON(Piece)
    ENABLE_INCR_OPERATORS_ON(PieceType)
    ENABLE_INCR_OPERATORS_ON(Square)
    ENABLE_INCR_OPERATORS_ON(File)
    ENABLE_INCR_OPERATORS_ON(Rank)
    ENABLE_INCR_OPERATORS_ON(Direction)
    ENABLE_INCR_OPERATORS_ON(Color)

    ENABLE_BASE_OPERATORS_ON(Square)
    ENABLE_BASE_OPERATORS_ON(File)
    ENABLE_BASE_OPERATORS_ON(Rank)

    ENABLE_BASE_OPERATORS_ON(Direction)

inline Direction operator*(int i, Direction d) { return Direction(i * int(d)); }        \
inline Direction operator*(Direction d, int i) { return Direction(int(d) * i); }        \

#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

#pragma endregion

}

#endif //MEETRA_TYPES_H
