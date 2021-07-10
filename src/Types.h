#ifndef MEETRA_TYPES_H
#define MEETRA_TYPES_H

#include <string>

namespace Meetra {

    typedef uint_fast16_t Depth;

    typedef int_fast16_t Score;

    typedef uint_fast64_t ZobristHash;

    enum GenPhase : uint8_t {
        BEST_MOVE, CAPTURE, QUIET, END
    };

    typedef uint64_t Bitboard;
#define EMPTY_BB 0UL

    enum Color : uint8_t {
        WHITE, BLACK,
        COLOR_NR
    };

    inline Color OtherColor(Color c) {
        return Color(c ^ BLACK);
    }

    enum PieceType : uint8_t {
        NONE_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_TYPES,
        PIECE_TYPE_NR
    };

    enum Piece : uint8_t {
        NO_PIECE,
        W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
        PIECE_NR = 15
    };

    inline Color ColorOfPiece(Piece p) { return static_cast<Color>(p >> 3); }
    inline PieceType TypeOfPiece(Piece p) { return static_cast<PieceType>(p & 7); }
    inline Piece NewPiece(PieceType pt, Color c) { return static_cast<Piece>((c << 3) | pt); }

    enum Rank : int8_t {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
        RANK_NR
    };

    enum File : int8_t {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
        FILE_NR
    };

    enum Direction : int8_t {
        NORTH = 8, NORTH_EAST = 9, EAST = 1, SOUTH_EAST = -7, SOUTH = -8, SOUTH_WEST = -9, WEST = -1, NORTH_WEST = 7,
        DIRECTION_NR = 8
    };

    enum DirectionIndex : uint8_t {
        NORTH_IDX, NORTH_EAST_IDX, EAST_IDX, SOUTH_EAST_IDX, SOUTH_IDX, SOUTH_WEST_IDX, WEST_IDX, NORTH_WEST_IDX,
        DIRECTION_IDX_NR
    };

    constexpr char FileNames[FILE_NR]{
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
    };
    constexpr char RankNames[RANK_NR]{
            '1', '2', '3', '4', '5', '6', '7', '8'
    };

    constexpr Direction Directions[DIRECTION_IDX_NR]{
            NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST
    };

    enum Square : int8_t {
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
    enum CastlingRights : uint16_t {
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
     * if the last (15th) bit is 1, it's a promotion move
     */
    typedef uint16_t Move;

    enum MoveType : uint16_t {
        INVALID_MOVE = 0, NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 13, PROMOTE_BISHOP = 5 << 13, PROMOTE_ROOK = 6 << 13, PROMOTE_QUEEN = 7 << 13,
        MOVE_TYPE_NR = 9
    };


    const std::string file_names = "abcdefgh";
    const std::string rank_names = "12345678";

#pragma region ===== Initialization =====
    inline Move NewMove(Square from, Square to) { return static_cast<Move>(from | to << 6); }
    inline Move NewMove(Square from, Square to, MoveType flag) { return static_cast<Move>(NewMove(from, to) | flag); }
    inline Move NewMoveFromName(const std::string &move_name) {
        File f_from = static_cast<File>(file_names.find(move_name[0]));
        Rank r_from = static_cast<Rank>((move_name[1] - '0') - 1);
        Square from = SquareFromFiRa(f_from, r_from);

        File f_to = static_cast<File>(file_names.find(move_name[2]));
        Rank r_to = static_cast<Rank>((move_name[3] - '0') - 1);
        Square to = SquareFromFiRa(f_to, r_to);

        const std::string asd = std::string("qwewq");

        MoveType flag = NO_FLAG;
        if (move_name.length() > 4) {
            switch (move_name[4]) {
                case 'q':
                    flag = PROMOTE_QUEEN;
                    break;
                case 'r':
                    flag = PROMOTE_ROOK;
                    break;
                case 'b':
                    flag = PROMOTE_BISHOP;
                    break;
                case 'n':
                    flag = PROMOTE_KNIGHT;
                    break;
            }
        }
        return NewMove(from, to, flag);
    }
#pragma endregion

#pragma region ===== Utils =====
    inline PieceType PieceTypeFromFlag(MoveType mt) { return static_cast<PieceType >((mt >> 13) - 2); }
    inline Square FromSquare(Move m) { return static_cast<Square>(m & 0x3F); }
    inline Square ToSquare(Move m) { return static_cast<Square>((m & 0xFC0) >> 6); }
    inline bool IsPromotion(Move m) { return m >> 15; }
    inline MoveType GetMoveType(Move m) { return static_cast<MoveType>(m & 0xF000); }
    inline bool IsValid(Move m) { return m != INVALID_MOVE; }
    inline std::string GetMoveName(Move m) {
        if (m == INVALID_MOVE) {
            return "0000";
        }
        std::string ret;
        ret.push_back(FileNames[FileFromSquare(FromSquare(m))]);
        ret.push_back(RankNames[RankFromSquare(FromSquare(m))]);
        ret.push_back(FileNames[FileFromSquare(ToSquare(m))]);
        ret.push_back(RankNames[RankFromSquare(ToSquare(m))]);
        if (IsPromotion(m)) {
            switch (GetMoveType(m)) {
                case PROMOTE_QUEEN:
                    ret.push_back('q');
                    break;
                case PROMOTE_ROOK:
                    ret.push_back('r');
                    break;
                case PROMOTE_BISHOP:
                    ret.push_back('b');
                    break;
                case PROMOTE_KNIGHT:
                    ret.push_back('n');
                    break;
                default:
                    break;
            }
        }
        return ret;
    }
    //inline constexpr bool IsValid(Move m) { return (m & 0x7FF) != INVALID_MOVE; }
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
    ENABLE_INCR_OPERATORS_ON(GenPhase)

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
