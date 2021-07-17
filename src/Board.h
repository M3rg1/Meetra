#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Types.h"
#include "Misc.h"
#include "ZobristHash.h"

namespace Meetra {

    class Board {

    public:
        Board();
        void NewPosition(const std::string &fen);
        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        [[nodiscard]] bool IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard SquareAttackers(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard PinnedPiecesForSquare(Square s, Color blockers_color) const;
        [[nodiscard]] int_fast16_t MovesMadeCount() const { return history_cnt; }

#pragma region ===== Piece getters =====
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetEmptySquares() const { return ~GetPieces(ALL_TYPES); }
        [[nodiscard]] inline PieceType GetPieceTypeOnSq(Square s) const { return TypeOfPiece(board[s]); }
#pragma endregion

#pragma region ===== Game State info getters =====
        [[nodiscard]] inline ZobristHash GetZobristHash() const { return current_state.zobrist_hash; }
        [[nodiscard]] inline CastlingRights GetCR() const { return static_cast<CastlingRights>(current_state.game_state & ALL_CR); }
        [[nodiscard]] inline bool CanWhiteShortCR() const { return (current_state.game_state & WHITE_SHORT) != 0; }
        [[nodiscard]] inline bool CanWhiteLongCR() const { return (current_state.game_state & WHITE_LONG) != 0; }
        [[nodiscard]] inline bool CanBlackShortCR() const { return (current_state.game_state & BLACK_SHORT) != 0; }
        [[nodiscard]] inline bool CanBlackLongCR() const { return (current_state.game_state & BLACK_LONG) != 0; }
        [[nodiscard]] inline bool CanColorCastleAny(Color c) const {
            return c == WHITE ? current_state.game_state & WHITE_ALL_CR : current_state.game_state & BLACK_ALL_CR;
        }
        [[nodiscard]] inline bool CanCastleAny() const { return current_state.game_state & ALL_CR; }
        [[nodiscard]] inline Square EpSquare() const { return static_cast<Square >(current_state.game_state & 0x3F); }
        [[nodiscard]] inline Color ColorToMove() const { return static_cast<Color>(current_state.game_state >> 10 & 0x1); }
        [[nodiscard]] inline Piece CapturedPiece() const { return static_cast<Piece>(current_state.game_state >> 11 & 0xF); }
        [[nodiscard]] inline int Ply() const { return static_cast<int>(current_state.game_state >> 15 & 0x3F); }
        [[nodiscard]] inline int TotalMoves() const { return static_cast<int>(current_state.game_state >> 22); }
        [[nodiscard]] inline bool IsRepetition() const {
            for (auto i = history_cnt - 2; i >= 0; i -= 2) {
                if (board_history[i].game_state & 0x7800) {
                    return false;
                } else if (board_history[i].zobrist_hash == current_state.zobrist_hash) {
                    return true;
                }
            }
            return false;
        }
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
        inline void SetEpSquare(Square s) { current_state.game_state |= static_cast<GameState>(s); }
        inline void SetCastlingRights(CastlingRights cr) { current_state.game_state |= static_cast<GameState>(cr); }
        inline void SetColorToMove(Color c) { current_state.game_state |= static_cast<GameState>(c << 10); }
        inline void SetCapturedPiece(Piece p) { current_state.game_state |= static_cast<GameState>(p << 11); }
        inline void SetPly(int ply) { current_state.game_state |= static_cast<GameState>(ply << 15); }
        inline void SetMoveNumber(int move_num) { current_state.game_state |= static_cast<GameState>(move_num << 22); }

        // modify current game state
        inline void ResetPly() { current_state.game_state &= static_cast<GameState>(~0x3F8000); }
        inline void RemoveCastlingRights(CastlingRights cr) { current_state.game_state &= static_cast<GameState>(~cr); }
        inline void IncrementMoveNumber(Color col_to_move) { current_state.game_state += col_to_move << 22; }
        inline void IncrementPly() { current_state.game_state += 1 << 15; }
        inline void ClearCapturedPiece() { current_state.game_state &= static_cast<GameState>(~0x7800); }
        inline void ChangeColorToMove() { current_state.game_state ^= 1 << 10; }
        inline void ClearEpSquare() { current_state.game_state &= static_cast<GameState>(~0x3F); }
#pragma endregion

#pragma region ===== Update inner structures =====
        void ParseFen(const std::string &fen);

        inline void RemovePiece(Square s, ZobristHash &h) {
            Piece p = board[s];
            board[s] = NO_PIECE;
            Bitboard pos = SquareToBB(s);
            color_bbs[ColorOfPiece(p)] ^= pos;
            type_bbs[TypeOfPiece(p)] ^= pos;
            type_bbs[ALL_TYPES] ^= pos;
            Zobrist::RemovePiece(h, TypeOfPiece(p), ColorOfPiece(p), s);
        }

        inline void PutPiece(Square s, Piece p, ZobristHash &h) {
            board[s] = p;
            Bitboard pos = SquareToBB(s);
            color_bbs[ColorOfPiece(p)] |= pos;
            type_bbs[TypeOfPiece(p)] |= pos;
            type_bbs[ALL_TYPES] |= pos;
            Zobrist::AddPiece(h, TypeOfPiece(p), ColorOfPiece(p), s);
        }

        inline void MovePiece(Square from, Square to, ZobristHash &h) {
            Piece p = board[from];
            board[to] = p;
            board[from] = NO_PIECE;
            Bitboard from_to = SquareToBB(from) | SquareToBB(to);
            color_bbs[ColorOfPiece(p)] ^= from_to;
            type_bbs[TypeOfPiece(p)] ^= from_to;
            type_bbs[ALL_TYPES] ^= from_to;
            Zobrist::MovePiece(h, TypeOfPiece(p), ColorOfPiece(p), from, to);
        }

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
        struct BoardData {
            GameState game_state;
            ZobristHash zobrist_hash;
        };

        BoardData board_history[MAX_GAME_LENGTH];
        size_t history_cnt;

        BoardData current_state;
        Piece board[SQUARE_NR];
        Bitboard color_bbs[COLOR_NR];
        Bitboard type_bbs[PIECE_TYPE_NR];
#pragma endregion
    };
}


#endif //MEETRA_BOARD_H
