#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Types.h"
#include "ZobristHash.h"
#include "Evaluator.h"

namespace Meetra {

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_LEGAL_MOVES 256
#define MAX_GAME_LENGTH 1024

    class Board {

    public:

        Board();
        bool NewPosition(const std::string &fen);
        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        bool MakeUciMove(const std::string &move_string);

        [[nodiscard]] bool IsMoveLegal(Move m) const;
        [[nodiscard]] bool IsBoardValid() const;
        [[nodiscard]] bool AllSquaresSafe(Bitboard squares, Color attacker, Bitboard occ) const;
        [[nodiscard]] bool IsAttackedByAny(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] bool IsAttackedBySliders(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard AttackedBy(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard PinnedToSquare(Square s, Color blockers_color) const;
        [[nodiscard]] std::string MoveToName(Move m) const;
        [[nodiscard]] Move MoveFromName(std::string_view move_name) const;
        [[nodiscard]] std::string PPBoard() const;

#pragma region ===== Getters =====
        [[nodiscard]] inline int HistorySize() const { return static_cast<int>(history_cnt); }
        [[nodiscard]] inline Score GetEval() const { return state.evaluator.GetBoardEval(); };
        [[nodiscard]] inline Score GetMoveEval(Move m) const { return state.evaluator.GetMoveEval(*this, m); };
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Piece GetPieceOnSquare(Square s) const { return board[s]; }
        [[nodiscard]] inline PieceType GetPieceTypeOnSq(Square s) const { return TypeOfPiece(GetPieceOnSquare(s)); }
        [[nodiscard]] inline Bitboard GetCr() const { return state.cr; }
        [[nodiscard]] inline Hash64 GetHash() const { return state.hash; }
        [[nodiscard]] inline bool CrAvailable(Color c, CastlingSide cs) const { return RookSqBB(c, cs); }
        [[nodiscard]] inline Square EpSquare() const { return state.ep_square; }
        [[nodiscard]] inline Color ColorToMove() const { return state.to_move; }
        [[nodiscard]] inline Piece CapturedPiece() const { return state.captured_piece; }
        [[nodiscard]] inline int Ply() const { return state.ply; }
        [[nodiscard]] inline int TotalMoves() const { return state.moves; }
        [[nodiscard]] inline bool Move50Rule() const { return Ply() >= 100; };
        [[nodiscard]] inline Bitboard RookSqBB(Color c, CastlingSide s) const { return state.cr & origin_rooks[c][s]; }
        [[nodiscard]] Move RookCastlingMove(Square king_to, Color c) const;
        [[nodiscard]] bool IsRepetition() const;
#pragma endregion

    private:

#pragma region ===== Game State modifications =====
        inline void SetEpSquare(Square s) { state.ep_square = s; }
        inline void SetColorToMove(Color c) { state.to_move = c; }
        inline void SetCapturedPiece(Piece p) { state.captured_piece = p; }
        inline void ResetPly() { state.ply = 0; }
        inline void IncrementMoveNumber(Color col_to_move) { state.moves += col_to_move; }
        inline void IncrementPly() { state.ply++; }
        inline void ClearCapturedPiece() { state.captured_piece = NO_PIECE; }
        inline void ChangeColorToMove() { state.to_move = static_cast<Color>(state.to_move ^ 1); }
        inline void ClearEpSquare() { state.ep_square = ZERO_SQ; }
#pragma endregion

#pragma region ===== Update inner structures =====
        bool ParseFen(const std::string &fen);
        void RemovePiece(Square s);
        void AddPiece(Square s, Piece p);
        void MovePiece(Square from, Square to);
#pragma endregion

#pragma region ===== Data =====
        struct BoardState {
            Hash64 hash;
            int ply = 0;
            int moves = 1;
            Color to_move = WHITE;
            Piece captured_piece = NO_PIECE;
            Square ep_square = ZERO_SQ;
            Bitboard cr = EMPTY_BB;
            Evaluation::Evaluator evaluator;
        };

        BoardState state;
        BoardState history[MAX_GAME_LENGTH];
        size_t history_cnt;

        // original positions of rooks that are available for castling
        Bitboard origin_rooks[COLOR_NR][CS_NR];

        Piece board[SQUARE_NR];
        Bitboard color_bbs[COLOR_NR];
        Bitboard type_bbs[PIECE_TYPE_NR + 1]; // + 1 for all_types
#pragma endregion
    };
}

#endif //MEETRA_BOARD_H
