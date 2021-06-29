#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Types.h"

namespace Meetra {

    class Board {

    public:
        explicit Board(std::string Fen);

        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        [[nodiscard]] bool IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard SquareAttackers(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard PinnedPiecesForSquare(Square s, Color blockers_color) const;

#pragma region ===== Piece getters =====
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetEmptySquares() const { return ~GetPieces(ALL_TYPES); }
#pragma endregion

#pragma region ===== Game State info getters =====
        [[nodiscard]] inline CastlingRights GetCR() const { return static_cast<CastlingRights>(game_state & ALL_CR); }
        [[nodiscard]] inline bool CanWhiteShortCR() const { return (game_state & WHITE_SHORT) != 0; }
        [[nodiscard]] inline bool CanWhiteLongCR() const { return (game_state & WHITE_LONG) != 0; }
        [[nodiscard]] inline bool CanBlackShortCR() const { return (game_state & BLACK_SHORT) != 0; }
        [[nodiscard]] inline bool CanBlackLongCR() const { return (game_state & BLACK_LONG) != 0; }
        [[nodiscard]] inline Square EpSquare() const { return static_cast<Square >(game_state & 0x3F); }
        [[nodiscard]] inline Color ColorToMove() const { return static_cast<Color>(game_state >> 10 & 0x1); }
        [[nodiscard]] inline Piece CapturedPiece() const { return static_cast<Piece>(game_state >> 11 & 0xF); }
        [[nodiscard]] inline int Ply() const { return static_cast<int>(game_state >> 15 & 0x3F); }
        [[nodiscard]] inline int TotalMoves() const { return static_cast<int>(game_state >> 22); }
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
        // bits 22+ - total moves made
        typedef uint32_t GameState;
#define NEW_GAME_STATE 0
#pragma endregion

#pragma region ===== Game State modifications =====
        // requires new game state
        inline void SetEpSquare(Square s) { game_state |= static_cast<GameState>(s); }
        inline void SetCastlingRights(CastlingRights cr) { game_state |= static_cast<GameState>(cr); }
        inline void SetColorToMove(Color c) { game_state |= static_cast<GameState>(c << 10); }
        inline void SetCapturedPiece(Piece p) { game_state |= static_cast<GameState>(p << 11); }
        inline void SetPly(int ply) { game_state |= static_cast<GameState>(ply << 15); }
        inline void SetMoveNumber(int move_num) { game_state |= static_cast<GameState>(move_num << 22); }

        // modify current game state
        inline void ResetPly() { game_state &= static_cast<GameState>(~0x3F8000); }
        inline void RemoveCastlingRights(CastlingRights cr) { game_state &= static_cast<GameState>(~cr); }
        inline void IncrementMoveNumber(uint32_t increment) { game_state += increment << 22; }
        inline void IncrementPly() { game_state += 1 << 15; }
        inline void ClearCapturedPiece() { game_state &= static_cast<GameState>(~0x7800); }
        inline void ChangeColorToMove() { game_state ^= 1 << 10; }
        inline void ClearEpSquare() { game_state &= static_cast<GameState>(~0x3F); }
#pragma endregion

#pragma region ===== Update inner structures =====
        inline void RemovePiece(Square s) {
            Piece p = board[s];
            board[s] = NO_PIECE;
            Bitboard pos = SquareToBB(s);
            color_bbs[ColorOfPiece(p)] ^= pos;
            type_bbs[TypeOfPiece(p)] ^= pos;
            type_bbs[ALL_TYPES] ^= pos;
        }

        inline void PutPiece(Square s, Piece p) {
            board[s] = p;
            Bitboard pos = SquareToBB(s);
            color_bbs[ColorOfPiece(p)] |= pos;
            type_bbs[TypeOfPiece(p)] |= pos;
            type_bbs[ALL_TYPES] |= pos;
        }

        inline void MovePiece(Square from, Square to) {
            Piece p = board[from];
            board[to] = p;
            board[from] = NO_PIECE;
            Bitboard from_to = SquareToBB(from) | SquareToBB(to);
            color_bbs[ColorOfPiece(p)] ^= from_to;
            type_bbs[TypeOfPiece(p)] ^= from_to;
            type_bbs[ALL_TYPES] ^= from_to;
        }
#pragma endregion

#pragma region ===== Data =====
/*        struct BoardData {
            GameState game_state;
            // zobrist_key
        };*/

        GameState history[256];
        uint8_t history_cnt;

        GameState game_state;
        Piece board[SQUARE_NR]{NO_PIECE};
        Bitboard color_bbs[COLOR_NR]{0};
        Bitboard type_bbs[PIECE_TYPE_NR]{NONE_PIECE_TYPE};
#pragma endregion
    };
}


#endif //MEETRA_BOARD_H
