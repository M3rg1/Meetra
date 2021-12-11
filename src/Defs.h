#ifndef MEETRA_DEFS_H
#define MEETRA_DEFS_H

#include <string>

using TimeRep = int64_t;

using Depth = int;

using Score = int;

using Hash64 = uint64_t;
using Hash16 = int;
constexpr Hash64 NEW_HASH64 = 0;

using Bitboard = uint64_t;
constexpr Bitboard EMPTY_BB = 0;
constexpr Bitboard FULL_BB = 0xFFFFFFFFFFFFFFFF;

using TTEpoch = int;
enum TTFlag {
    NOT_FOUND, UPPER, LOWER, EXACT, CUTOFF
};

enum GenType {
    QSEARCH, NORMAL
};

enum GenMode {
    GENERATE, VALIDATE
};

enum GenPhase {
    PROMOTION, CAPTURE, QUIET, END, DOUBLE_CHECK
};

enum Node {
    PV, NON_PV, NULL_MOVE
};

enum Color {
    WHITE, BLACK,
    COLOR_NR
};
constexpr Color Colors[] = {WHITE, BLACK};
constexpr Color OtherColor(Color c) { return static_cast<Color>(c ^ 1); }

enum PieceType {
    NONE_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NR,
    ALL_TYPES = 7
};
constexpr PieceType PieceTypes[] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

enum Piece {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
};

constexpr Color ColorOfPiece(Piece p) { return static_cast<Color>(p >> 3); }
constexpr PieceType TypeOfPiece(Piece p) { return static_cast<PieceType>(p & 7); }
constexpr Piece NewPiece(PieceType pt, Color c) { return static_cast<Piece>((c << 3) | pt); }

constexpr std::string_view piece_char = "oPNBRQK  pnbrqk";

constexpr Piece CharToPiece(char c) { return static_cast<Piece>(piece_char.find(c)); }
constexpr char PieceToChar(Piece p) { return piece_char[p]; }

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
    RANK_NR
};
constexpr Rank Ranks[] = {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_NR
};
constexpr File Files[] = {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr int operator+(File f, Rank r) { return static_cast<int>(f) + static_cast<int>(r); }
constexpr int operator+(Rank r, File f) { return f + r; }
constexpr int operator-(File f, Rank r) { return static_cast<int>(f) - static_cast<int>(r); }
constexpr int operator-(Rank r, File f) { return f - r; }

constexpr Rank CharToRank(char c) { return static_cast<Rank>(c - '1'); }
constexpr char RankToChar(Rank r) { return static_cast<char>(r + '1'); }
constexpr File CharToFile(char c) { return static_cast<File>(c - 'a'); }
constexpr char FileToChar(File f) { return static_cast<char>(f + 'a'); }

enum PawnMoveDir {
    LEFT, RIGHT, FORWARD,
};

enum Direction : int {
    NORTH = 8, NORTH_EAST = 9, EAST = 1, SOUTH_EAST = -7, SOUTH = -8, SOUTH_WEST = -9, WEST = -1, NORTH_WEST = 7
};
constexpr Direction operator+(Direction d1, Direction d2) { return static_cast<Direction>(static_cast<int>(d1) + static_cast<int>(d2)); }
constexpr Direction operator-(Direction d1, Direction d2) { return static_cast<Direction>(static_cast<int>(d1) - static_cast<int>(d2)); }
constexpr Direction operator*(int i, Direction d) { return static_cast<Direction>(static_cast<int>(d) * i); }

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
    NO_SQ = 0
};
constexpr Square Squares[] = {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8
};
constexpr Square operator+(Square s, int i) { return static_cast<Square>(static_cast<int>(s) + i); }
constexpr Square operator-(Square s, int i) { return static_cast<Square>(static_cast<int>(s) - i); }
constexpr Square &operator+=(Square &s, int i) { return s = s + i; }
constexpr Square &operator-=(Square &s, int i) { return s = s - i; }

constexpr Bitboard SqToBB(Square s) { return 0x1ULL << s; }
constexpr Square FiRaToSq(File f, Rank r) { return static_cast<Square>((r << 3) | f); }
constexpr File SqToFile(Square s) { return static_cast<File>(s & 7); }
constexpr Rank SqToRank(Square s) { return static_cast<Rank>(s >> 3); }
constexpr Square StrToSq(std::string_view name) { return FiRaToSq(CharToFile(name[0]), CharToRank(name[1])); }
inline std::string SqToStr(Square s) { return {FileToChar(SqToFile(s)), RankToChar(SqToRank(s))}; }

enum CastlingSide {
    SHORT, LONG, CS_NR
};

/**
 * Move is 16 bits (for TT storing) :
 * 0-5 from square
 * 6-11 to square
 * 12-15 MoveType flag
 * if the 14th bit is 1, it's a promotion move -> prom bits  N = 0010.., B = 1010.., R = 0110.., Q = 1110..
 */
using Move = int;
constexpr Move ZERO_MOVE = 0;

enum MoveType {
    NO_FLAG = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
    PROMOTE_KNIGHT = 4 << 12, PROMOTE_BISHOP = 5 << 12, PROMOTE_ROOK = 6 << 12, PROMOTE_QUEEN = 7 << 12
};

constexpr Move NewMove(Square from, Square to) { return from | (to << 6); }
constexpr Move NewMove(Square from, Square to, MoveType flag) { return NewMove(from, to) | flag; }

constexpr PieceType PromotionTo(MoveType mt) { return static_cast<PieceType>((mt >> 12) - 2); }
constexpr Square FromSquare(Move m) { return static_cast<Square>(m & 0x3F); }
constexpr Square ToSquare(Move m) { return static_cast<Square>((m >> 6) & 0x3F); }
constexpr bool IsPromotion(Move m) { return m >> 14; }
constexpr MoveType GetMoveType(Move m) { return static_cast<MoveType>(m & 0xF000); }
#endif //MEETRA_DEFS_H
