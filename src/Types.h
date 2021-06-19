#ifndef POPPER_TYPES_H
#define POPPER_TYPES_H

#include <cinttypes>

namespace Popper {

    typedef uint64_t Bitboard;

    enum Color : int {
        WHITE, BLACK,
        COLOR_NR
    };

    enum PieceType : int {
        NONE_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_TYPES,
        PIECE_TYPE_NR
    };

    enum Piece : int {
        NO_PIECE,
        W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
        PIECE_NR
    };

    enum Rank : int {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
        RANK_NR
    };

    enum File : int {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
        FILE_NR
    };

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *  Ten bitboard funguje DOBRE:
     *
     *
     *
     *  0 0 0 0 0 0 0 0
     *  0 0 0 0 0 0 0 0
     *  ^
     *  A1 = prvni cislo bitboardu kdyz dam LSB_POP
     *
     *  a potom H8 je posledni LSB pop
     *  proste ten bitboard jde takhle
     *
     *  > > > > > > > >
     *  > > > > > > > >
     *  8 9 ....
     *  0 1 2 3 4 5 6 7
     *  = little endian
     *  na miste s nejnizsi adresou (ten BAJT nejvic vpravo) je nejmene vyznamny bajn
     *  tj proste klasika, prvni rada odspoda je prvni bajt, druha drhuhy atd.
     *  a zleva dopraprava jsou jednotlive bity v tom bajtu
     */


    // potom v bitboardu
    // A1 = 0, B1 = 1 ...
    enum Square : int {
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

    inline constexpr Square SquareFromFiRa(File f, Rank r) { return Square((r << 3) + f); }


    enum GenPhase : int {
        BEST_MOVE, CAPTURE, QUIET, END,
        GEN_PHASE_NR
    };

    enum RankMask : Bitboard {
        RANK_ONE_MASK = 0x00000000000000FFUL,
        RANK_TWO_MASK = 0x000000000000FF00UL,
        RANK_THREE_MASK = 0x0000000000FF0000UL,
        RANK_FOUR_MASK = 0x00000000FF000000UL,
        RANK_FIVE_MASK = 0x000000FF00000000UL,
        RANK_SIX_MASK = 0x0000FF0000000000UL,
        RANK_SEVEN_MASK = 0x00FF000000000000UL,
        RANK_EIGHT_MASk = 0xFF00000000000000UL
    };

    enum FileMask : Bitboard {
        FILE_A_MASK = 0x8080808080808080UL,
        FILE_B_MASK = 0x4040404040404040UL,
        FILE_C_MASK = 0x2020202020202020UL,
        FILE_D_MASK = 0x1010101010101010UL,
        FILE_E_MASK = 0x0808080808080808UL,
        FILE_F_MASK = 0x0404040404040404UL,
        FILE_G_MASK = 0x0202020202020202UL,
        FILE_H_MASK = 0x0101010101010101UL
    };

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); }    \
constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); }    \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }       \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }           \
//inline T operator++(T& d)(int) { return d = T(int(d) + 1); }           \
//inline T& operator--(int)(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

    ENABLE_INCR_OPERATORS_ON(Piece)
    ENABLE_INCR_OPERATORS_ON(PieceType)
    ENABLE_INCR_OPERATORS_ON(Square)
    ENABLE_INCR_OPERATORS_ON(File)
    ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON
    /**
     * Move bits:
     * 0-5 from square
     * 6-11 to square
     * 12-15 MoveType flag
     * if the last (15th) bit is 1, it's a promotion move
     */
    typedef uint16_t Move;

    enum MoveType : int {
        INVALID_MOVE = 0, EN_PASSANT = 1 << 12, CASTLING = 2 << 12, TWO_FORWARD = 3 << 12,
        PROMOTE_KNIGHT = 4 << 13, PROMOTE_BISHOP = 5 << 13, PROMOTE_ROOK = 6 << 13, PROMOTE_QUEEN = 7 << 13,
        MOVE_TYPE_NR
    };
    // Move initializers
    inline constexpr Move NewMove(Square from, Square to) { return from | to << 6; }
    inline constexpr Move NewMove(Square from, Square to, MoveType flag) { return NewMove(from, to) | flag; }

    // Move utils
    inline constexpr Square FromSquare(Move m) { return Square(m & 0x3F); }
    inline constexpr Square ToSquare(Move m) { return Square((m & 0xFC0) >> 6); }
    inline constexpr bool IsPromotion(Move m) { return m >> 15 != 0; }
    inline constexpr MoveType GetFlag(Move m) { return MoveType(m & 0xF000); }
    inline constexpr bool IsValid(Move m) { return m != INVALID_MOVE; }
    //inline constexpr bool IsValid(Move m) { return (m & 0x7FF) != 0; }


    enum CastlingRights : int {
        NO_CASTLING = 0, WHITE_SHORT = 1 << 6, WHITE_LONG = 1 << 7, BLACK_SHORT = 1 << 8, BLACK_LONG = 1 << 9,
        WHITE_ALL_CR = WHITE_SHORT | WHITE_LONG,
        BLACK_ALL_CR = BLACK_LONG | BLACK_SHORT,
        ALL_CR = WHITE_SHORT | WHITE_LONG | BLACK_SHORT | BLACK_LONG,
        CASTLING_RIGHTS_NR
    };

    // from right to left
    // bits 0-5 = enpassant square index
    // bits 6-7 = castling rights white
    // bits 7-9 = castling rights black
    // bit  10 = player to move
    // bits 11-17 = captured piece (from last game state to this game state)
    // bits 17-23 = ply since last capture/pawn moves - 50 move rule
    // bits 24+ - total moves made
    typedef uint_fast32_t GameState;
#define NEW_GAME_STATE 0

    // game state setters
    // requires new game state
    inline constexpr void SetEpSquare(Square s, GameState &gs) { gs |= s; }
    inline constexpr void SetCastlingRights(CastlingRights cr, GameState &gs) { gs |= cr; }
    inline constexpr void SetColorToMove(Color c, GameState &gs) { gs |= c << 10; }
    inline constexpr void SetCapturedPiece(Color c, GameState &gs) { gs |= c << 11; }
    inline constexpr void SetPly(int ply, GameState &gs) { gs |= ply << 17; }
    inline constexpr void SetMoveNumber(int move_num, GameState &gs) { gs |= move_num << 24; }

    // game state getters
    //inline constexpr CastlingRights GetCR(GameState &gs){ return CastlingRights (gs & 0x3C0); }
    inline constexpr bool CanWhiteShortCR(GameState &gs) { return (gs & WHITE_SHORT) != 0; }
    inline constexpr bool CanWhiteLongCR(GameState &gs) { return (gs & WHITE_LONG) != 0; }
    inline constexpr bool CanBlackShortCR(GameState &gs) { return (gs & BLACK_SHORT) != 0; }
    inline constexpr bool CanBlackLongCR(GameState &gs) { return (gs & BLACK_LONG) != 0; }
    inline constexpr Square EpSquare(GameState &gs) { return Square(gs & 0x1F); }
    inline constexpr Color ColorToMove(GameState &gs) { return Color(gs >> 10 & 0x1); }
    inline constexpr Piece CapturedPiece(GameState &gs) { return Piece(gs >> 11 & 0x3F); }
    inline constexpr int Ply(GameState &gs) { return int(gs >> 17 & 0x3F); }
    inline constexpr int TotalMoves(GameState &gs) { return int(gs >> 24); }

    // changing existing, non-new game state
    inline constexpr void IncrementMoveNumber(GameState &gs) { gs += 1 << 23; }
    inline constexpr void IncrementPly(GameState &gs) { gs += 1 << 17; }
    inline constexpr void ClearCapturedPiece(GameState &gs) { gs &= ~0x1F800; }
    inline constexpr void ChangeColorToMove(GameState &gs) {
        ColorToMove(gs) == WHITE ? gs += 1 << 10 : gs -= 1 << 10;
    }


}

#endif //POPPER_TYPES_H
