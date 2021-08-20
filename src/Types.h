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

    enum GenPhase : uint_fast8_t {
        CAPTURE, QUIET, END, DOUBLE_CHECK
    };

    enum Color : uint_fast8_t {
        WHITE, BLACK,
        COLOR_NR
    };

    inline Color OtherColor(Color c) {
        return static_cast<Color>(!c);
    }

    enum PieceType : uint_fast8_t {
        NONE_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_TYPES,
        PIECE_TYPE_NR
    };

    enum Piece : uint_fast8_t {
        NO_PIECE,
        W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    };

    inline Color ColorOfPiece(Piece p) { return static_cast<Color>(p >> 3); }
    inline PieceType TypeOfPiece(Piece p) { return static_cast<PieceType>(p & 7); }
    inline Piece NewPiece(PieceType pt, Color c) { return static_cast<Piece>((c << 3) | pt); }

    enum Rank : int_fast8_t {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
        RANK_NR
    };

    enum File : int_fast8_t {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
        FILE_NR
    };

    inline Rank RankFromChar(char c) { return static_cast<Rank>(c - '1'); }
    inline char CharFromRank(Rank r) { return static_cast<char>(r + '1'); }
    inline File FileFromChar(char c) { return static_cast<File>(c - 'a'); }
    inline char CharFromFile(File f) { return static_cast<char>(f + 'a'); }


    enum PawnMoveDir : int_fast8_t {
        LEFT, RIGHT, ONE_FWD, TWO_FWD
    };

    enum Direction : int_fast8_t {
        NORTH = 8, NORTH_EAST = 9, EAST = 1, SOUTH_EAST = -7, SOUTH = -8, SOUTH_WEST = -9, WEST = -1, NORTH_WEST = 7
    };

    enum DirectionIndex : uint_fast8_t {
        NORTH_IDX, NORTH_EAST_IDX, EAST_IDX, SOUTH_EAST_IDX, SOUTH_IDX, SOUTH_WEST_IDX, WEST_IDX, NORTH_WEST_IDX,
        DIRECTION_IDX_NR
    };

    constexpr Direction Directions[DIRECTION_IDX_NR]{
            NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST
    };

    enum Square : uint_fast8_t {
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

    inline Square SquareFromFiRa(File f, Rank r) { return static_cast<Square>((r << 3) | f); }
    inline File FileFromSquare(Square s) { return static_cast<File>(s & 7); }
    inline Rank RankFromSquare(Square s) { return static_cast<Rank>(s >> 3); }
    inline Bitboard SquareToBB(Square s) { return static_cast<Bitboard>(0x1) << s; }

#pragma region ===== Castling related stuff =====
    enum CastlingRights : uint_fast16_t {
        NO_CASTLING = 0, WHITE_SHORT = 1 << 6, WHITE_LONG = 1 << 7, BLACK_SHORT = 1 << 8, BLACK_LONG = 1 << 9,
        WHITE_ALL_CR = WHITE_SHORT | WHITE_LONG,
        BLACK_ALL_CR = BLACK_LONG | BLACK_SHORT,
        ALL_CR = WHITE_SHORT | WHITE_LONG | BLACK_SHORT | BLACK_LONG
    };

    constexpr CastlingRights castling_mask[SQUARE_NR]{
            WHITE_LONG, NO_CASTLING, NO_CASTLING, NO_CASTLING, WHITE_ALL_CR, NO_CASTLING, NO_CASTLING, WHITE_SHORT,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
            BLACK_LONG, NO_CASTLING, NO_CASTLING, NO_CASTLING, BLACK_ALL_CR, NO_CASTLING, NO_CASTLING, BLACK_SHORT
    };

    inline Square RookFromCastling(Square king_to) {
        return king_to == G1 ? H1 : king_to == G8 ? H8 : king_to == C1 ? A1 : king_to == C8 ? A8 : SQUARE_ZERO;
    }

    inline Square RookToCastling(Square king_to) {
        return king_to == G1 ? F1 : king_to == G8 ? F8 : king_to == C1 ? D1 : king_to == C8 ? D8 : SQUARE_ZERO;
    }
#pragma endregion

#pragma region ===== Move =====
    /**
     * Move bits:
     * 0-5 from square
     * 6-11 to square
     * 12-15 MoveType flag
     * if the last (15th) bit is 1, it's a promotion move -> prom bits  N = 100, B = 101, R = 110, Q = 111
     */
    typedef uint16_t Move;

    enum MoveType : uint_fast16_t {
        ZERO_MOVE = 0, NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 13, PROMOTE_BISHOP = 5 << 13, PROMOTE_ROOK = 6 << 13, PROMOTE_QUEEN = 7 << 13
    };

#pragma region ===== Initialization =====
    inline Move NewMove(Square from, Square to) { return static_cast<Move>(from | to << 6); }
    inline Move NewMove(Square from, Square to, MoveType flag) { return static_cast<Move>(NewMove(from, to) | flag); }
    inline Move NewMoveFromName(const std::string &move_name) {

        Square s_from = SquareFromFiRa(FileFromChar(move_name[0]), RankFromChar(move_name[1]));
        Square s_to = SquareFromFiRa(FileFromChar(move_name[2]),  RankFromChar(move_name[3]));

        MoveType flag = NO_FLAG;
        if (move_name.length() > 4) {
            flag = move_name[4] == 'q' ? PROMOTE_QUEEN :
                   move_name[4] == 'r' ? PROMOTE_ROOK :
                   move_name[4] == 'b' ? PROMOTE_BISHOP :
                   PROMOTE_KNIGHT;
        }

        return NewMove(s_from, s_to, flag);
    }
#pragma endregion

#pragma region ===== Utils =====

    inline PieceType PieceTypeFromFlag(MoveType mt) { return static_cast<PieceType >((mt >> 13) - 2); }
    inline Square FromSquare(Move m) { return static_cast<Square>(m & 0x3F); }
    inline Square ToSquare(Move m) { return static_cast<Square>((m & 0xFC0) >> 6); }
    inline bool IsPromotion(Move m) { return m >> 15; }
    inline MoveType GetMoveType(Move m) { return static_cast<MoveType>(m & 0xF000); }
    inline bool IsValidMove(Move m) { return m != ZERO_MOVE; }
    inline std::string GetMoveName(Move m) {

        if (m == ZERO_MOVE) {
            return "0000";
        }

        std::string ret = {
                CharFromFile(FileFromSquare(FromSquare(m))),
                CharFromRank(RankFromSquare(FromSquare(m))),
                CharFromFile(FileFromSquare(ToSquare(m))),
                CharFromRank(RankFromSquare(ToSquare(m)))
        };

        if (IsPromotion(m)) {
            MoveType prom_flag = GetMoveType(m);
            ret += prom_flag == PROMOTE_QUEEN ? 'q' :
                   prom_flag == PROMOTE_ROOK ? 'r' :
                   prom_flag == PROMOTE_BISHOP ? 'b' :
                   'n';
        }

        return ret;
    }
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
    ENABLE_INCR_OPERATORS_ON(DirectionIndex)
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
