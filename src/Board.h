#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H


#include <string>
#include "Types.h"
#include <deque>
#include <chrono>


namespace Meetra {

    class Board {

    public:
        explicit Board(std::string Fen);

        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        [[nodiscard]] Bitboard SquareAttackers(Square s, Color c, Bitboard occ) const;

#pragma region ===== Piece getters =====
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetCheckers() const { return checkers; }
        [[nodiscard]] inline Bitboard GetEmptySquares() const { return ~GetPieces(ALL_TYPES); }
#pragma endregion

#pragma region ===== Game State info getters =====
        [[nodiscard]] inline bool CanWhiteShortCR() const { return (game_state & WHITE_SHORT) != 0; }
        [[nodiscard]] inline bool CanWhiteLongCR() const { return (game_state & WHITE_LONG) != 0; }
        [[nodiscard]] inline bool CanBlackShortCR() const { return (game_state & BLACK_SHORT) != 0; }
        [[nodiscard]] inline bool CanBlackLongCR() const { return (game_state & BLACK_LONG) != 0; }
        [[nodiscard]] inline Square EpSquare() const { return static_cast<Square >(game_state & 0x3F); }
        [[nodiscard]] inline Color ColorToMove() const { return static_cast<Color>(game_state >> 10 & 0x1); }
        [[nodiscard]] inline Piece CapturedPiece() const { return static_cast<Piece>(game_state >> 11 & 0xF); }
        [[nodiscard]] inline int Ply() const { return static_cast<int>(game_state >> 15 & 0x3F); }
        [[nodiscard]] inline int TotalMoves() const { return static_cast<int>(game_state >> 21); }
#pragma endregion

#pragma region ===== Misc =====
        [[nodiscard]] std::string PPBoard() const;
#pragma endregion


    private:
#pragma region ===== Game State definitions =====
        // from right to left
        // bits 0-5 = ep square index
        // bits 6-7 = castling rights white
        // bits 7-9 = castling rights black
        // bit  10 = player to move
        // bits 11-14 = captured piece (from last game state to this game state)
        // bits 15-21 = ply since last capture/pawn moves - 50 move rule
        // bits 21+ - total moves made
        typedef uint_fast32_t GameState;
#define NEW_GAME_STATE 0

        enum CastlingRights : int {
            NO_CASTLING = 0, WHITE_SHORT = 1 << 6, WHITE_LONG = 1 << 7, BLACK_SHORT = 1 << 8, BLACK_LONG = 1 << 9,
            WHITE_ALL_CR = WHITE_SHORT | WHITE_LONG,
            BLACK_ALL_CR = BLACK_LONG | BLACK_SHORT,
            ALL_CR = WHITE_SHORT | WHITE_LONG | BLACK_SHORT | BLACK_LONG,
            CASTLING_RIGHTS_NR
        };
#pragma endregion

#pragma region ===== Game State modifications =====
        // requires new game state
        inline void SetEpSquare(Square s) { game_state |= static_cast<GameState>(s); }
        inline void SetCastlingRights(CastlingRights cr) { game_state |= static_cast<GameState>(cr); }
        inline void SetColorToMove(Color c) { game_state |= static_cast<GameState>(c << 10); }
        inline void SetCapturedPiece(Piece p) { game_state |= static_cast<GameState>(p << 11); }
        inline void SetPly(int ply) { game_state |= static_cast<GameState>(ply << 15); }
        inline void SetMoveNumber(int move_num) { game_state |= static_cast<GameState>(move_num << 21); }

        // modify current game state
        inline void ResetPly() { game_state &= static_cast<GameState>(~0x3F8000); }
        inline void RemoveCastlingRights(CastlingRights cr) { game_state &= static_cast<GameState>(~cr); }
        inline void IncrementMoveNumber() { game_state += 1 << 21; }
        inline void IncrementPly() { game_state += 1 << 15; }
        inline void ClearCapturedPiece() { game_state &= static_cast<GameState>(~0x7800); }
        inline void ChangeColorToMove() { ColorToMove() == WHITE ? game_state += 1 << 10 : game_state -= 1 << 10; }
        inline void ClearEpSquare() { game_state &= static_cast<GameState>(~0x3F); }
#pragma endregion

#pragma region ===== Update inner structures =====
        inline void MovePiece(Square from, Square to);
        inline void RemovePiece(Square s);
        inline void PutPiece(Square s, Piece p);
#pragma endregion

#pragma region ===== Data =====
        struct BoardData {
            GameState game_state;
            // zobrist_key
            Bitboard checkers;
        };

        std::deque<BoardData> gs_history;
        GameState game_state;
        Piece board[SQUARE_NR]{NO_PIECE};
        Bitboard color_bbs[COLOR_NR]{0};
        Bitboard type_bbs[PIECE_TYPE_NR]{0};
        Bitboard checkers;

#pragma endregion
    };
}


#endif //MEETRA_BOARD_H
