#ifndef MEETRA_TYPES_H
#define MEETRA_TYPES_H

#include <string>

namespace Meetra {

    typedef uint_fast8_t Epoch;

    typedef uint_fast8_t Depth;

    typedef int_fast16_t Score;

    typedef uint_fast64_t ZobristHash;
#define NEW_HASH 0

    typedef uint_fast64_t Key32;

    typedef uint64_t Bitboard;
#define EMPTY_BB 0

    enum GenPhase {
        CAPTURE, QUIET, END, DOUBLE_CHECK
    };

    enum Color {
        WHITE, BLACK,
        COLOR_NR
    };

    inline Color OtherColor(Color c) {
        return static_cast<Color>(!c);
    }

    enum PieceType {
        NONE_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6, ALL_TYPES,
        PIECE_TYPE_NR = 7
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

    constexpr std::string_view piece_char = "oPNBRQK  pnbrqk";

    inline Piece CharToPiece(char c) {
        // if not found -> std::string:npos + 1 == 0 == NO_PIECE, else -> index == desired piece
        return static_cast<Piece>(piece_char.find(c));
    }

    inline char PieceToChar(Piece p) {
        return piece_char[p];
    }

    enum PawnMoveDir {
        LEFT, RIGHT, ONE_FWD, TWO_FWD
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
        SQUARE_ZERO = 0,
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

#pragma region ===== Castling related stuff =====
    enum CastlingSide {
        SHORT, LONG
    };

#pragma endregion

#pragma region ===== Move =====
    /**
     * Move bits:
     * 0-5 from square
     * 6-11 to square
     * 12-15 MoveType flag
     * if the 14th bit is 1, it's a promotion move -> prom bits  N = 0010, B = 1010, R = 0110, Q = 0111
     */
     // MMMMMMMM MMMMFFFF
     // 00000000 00001111
    typedef uint16_t Move;

    enum MoveType {
        ZERO_MOVE = 0, NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 12, PROMOTE_BISHOP = 5 << 12, PROMOTE_ROOK = 6 << 12, PROMOTE_QUEEN = 7 << 12
    };

#pragma region ===== Initialization =====
    inline Move NewMove(Square from, Square to) { return static_cast<Move>(from | to << 6); }
    inline Move NewMove(Square from, Square to, MoveType flag) { return static_cast<Move>(NewMove(from, to) | flag); }
    // Make a move from UCI move string, if the move is a promotion, it will set the appropriate flag,
    // however for non-promotion special moves (castling, two forward ...) it wont.
#pragma endregion

#pragma region ===== Utils =====


    template<Color C>
    inline int IdxFromPieceType(PieceType pt) { return C == WHITE ? pt - 1 : pt + 5; }
    inline int IdxFromPiece(Piece p) { return ColorOfPiece(p) == WHITE ? p - 1 : p - 3; }
    inline PieceType PieceTypeFromFlag(MoveType mt) { return static_cast<PieceType >((mt >> 12) - 2); }
    inline Square FromSquare(Move m) { return static_cast<Square>(m & 0x3F); }
    inline Square ToSquare(Move m) { return static_cast<Square>((m & 0xFC0) >> 6); }
    inline bool IsPromotion(Move m) { return m >> 14; }
    inline MoveType GetMoveType(Move m) { return static_cast<MoveType>(m & 0xF000); }

#pragma endregion
#pragma endregion


#pragma region ===== Operator overloading settings =====

#define ENABLE_BASE_OPERATORS_ON(T)                                \
inline T operator+(T d1, int d2) { return T(int(d1) + d2); }    \
inline T operator-(T d1, int d2) { return T(int(d1) - d2); }    \
inline T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }       \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }           \

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
inline T operator*(int i, T d) { return T(i * int(d)); }        \
inline T operator*(T d, int i) { return T(int(d) * i); }        \
inline T operator/(T d, int i) { return T(int(d) / i); }        \
inline int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

    ENABLE_BASE_OPERATORS_ON(Square)

    ENABLE_INCR_OPERATORS_ON(Piece)
    ENABLE_INCR_OPERATORS_ON(PieceType)
    ENABLE_INCR_OPERATORS_ON(Square)
    ENABLE_INCR_OPERATORS_ON(File)
    ENABLE_INCR_OPERATORS_ON(Rank)
    ENABLE_INCR_OPERATORS_ON(Direction)
    //ENABLE_INCR_OPERATORS_ON(GenPhase)

    ENABLE_FULL_OPERATORS_ON(File)
    ENABLE_FULL_OPERATORS_ON(Rank)
    ENABLE_FULL_OPERATORS_ON(Direction)

    /// Additional operators to add a Direction to a Square
    inline Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
    inline Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
    inline Square &operator+=(Square &s, Direction d) { return s = s + d; }
    inline Square &operator-=(Square &s, Direction d) { return s = s - d; }

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON


#pragma endregion

}

#endif //MEETRA_TYPES_H
