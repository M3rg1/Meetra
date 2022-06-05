#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Defs.h"
#include "ZobristHash.h"
#include "Evaluator.h"
#include "Config.h"
#include "Bitboards.h"

class Board {

public:

    Board();
    bool NewPosition(const std::string &fen, bool isChess960 = false);
    bool MakeMove(Move m);
    void UnmakeMove(Move m);
    void MakeNullMove();
    void UnmakeNullMove();
    bool MakeUciMove(std::string_view move);

    [[nodiscard]] bool IsMoveLegal(Move m) const;
    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] bool AllSquaresSafe(Bitboard squares, Color attacker, Bitboard occ) const;
    [[nodiscard]] bool IsAttackedByAny(Square s, Color attacked_by, Bitboard occ) const;
    [[nodiscard]] bool IsAttackedBySliders(Square s, Color attacked_by, Bitboard occ) const;
    [[nodiscard]] Bitboard AttackedBy(Square s, Color attacked_by, Bitboard occ) const;
    [[nodiscard]] Bitboard PinnedToSquare(Square s, Color blockers_color) const;
    [[nodiscard]] std::string MoveToStr(Move m) const;
    [[nodiscard]] Move StrToMove(std::string_view move_str) const;
    [[nodiscard]] std::string Fen() const;
    [[nodiscard]] inline bool IsQuiet(Move m) const {
        return TypeOfMove(m) != EN_PASSANT && !IsPromotion(m) && PieceOnSquare(ToSquare(m)) == NO_PIECE;
    }

#pragma region ===== Getters =====
    [[nodiscard]] inline int HistorySize() const { return history_cnt; }
    [[nodiscard]] inline Score Eval() const { return state.evaluator.BoardEval(); };
    [[nodiscard]] inline Score MoveEval(Move m) const { return state.evaluator.MoveEval(*this, m); };
    [[nodiscard]] inline Bitboard Pieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
    [[nodiscard]] inline Bitboard Pieces(PieceType pt) const { return type_bbs[pt]; }
    [[nodiscard]] inline Bitboard Pieces(Color c) const { return color_bbs[c]; }
    [[nodiscard]] inline Piece PieceOnSquare(Square s) const { return board[s]; }
    [[nodiscard]] inline PieceType PieceTypeOnSq(Square s) const { return TypeOfPiece(PieceOnSquare(s)); }
    [[nodiscard]] inline Bitboard Cr() const { return state.cr; }
    [[nodiscard]] inline Hash64 Hash() const { return state.hash; }
    [[nodiscard]] inline bool IsCastlingAvailable(Color c, CastlingSide cs) const { return RookSqBB(c, cs); }
    [[nodiscard]] inline Square EpSquare() const { return state.ep_square; }
    [[nodiscard]] inline Color ColorToMove() const { return state.to_move; }
    [[nodiscard]] inline Piece CapturedPiece() const { return state.captured_piece; }
    [[nodiscard]] inline int Ply() const { return state.ply; }
    [[nodiscard]] inline int FullMoveClock() const { return state.moves; }
    [[nodiscard]] inline bool IsMove50Rule() const { return Ply() >= 100; };
    [[nodiscard]] inline Bitboard RookSqBB(Color c, CastlingSide s) const { return state.cr & origin_rooks[c][s]; }
    [[nodiscard]] inline Square RookSq(Color c, CastlingSide s) const { return Bitboards::Lsb(RookSqBB(c, s)); }
    [[nodiscard]] inline int Phase() const { return state.evaluator.Phase(); }
    [[nodiscard]] inline bool IsInCheck() const { return state.checkers; }
    [[nodiscard]] inline Bitboard Checkers() const { return state.checkers; }
    [[nodiscard]] inline bool IsChess960() const { return chess960; }
    [[nodiscard]] Move RookCastlingMove(Square king_to, Color c) const;
    [[nodiscard]] bool IsDrawByMaterial() const;
    [[nodiscard]] bool AnyLegalMoves() const;
    [[nodiscard]] bool IsRepetition() const;
    [[nodiscard]] bool IsDraw() const;
#pragma endregion

private:

    friend std::ostream &operator<<(std::ostream &os, const Board &board);

#pragma region ===== Game State modifications =====
    inline void SetEpSquare(Square s) { state.ep_square = s; }
    inline void SetColorToMove(Color c) { state.to_move = c; }
    inline void SetCapturedPiece(Piece p) { state.captured_piece = p; }
    inline void ResetPly() { state.ply = 0; }
    inline void IncrementMoveNumber(Color col_to_move) { state.moves += col_to_move; }
    inline void IncrementPly() { ++state.ply; }
    inline void ClearCapturedPiece() { state.captured_piece = NO_PIECE; }
    inline void ChangeColorToMove() { state.to_move = OtherColor(state.to_move); }
    inline void ClearEpSquare() { state.ep_square = NO_SQ; }
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
        Square ep_square = NO_SQ;
        Bitboard cr = EMPTY_BB;
        Bitboard checkers = EMPTY_BB;
        Evaluator evaluator;
    };

    bool chess960 = false;

    BoardState state;
    std::array<BoardState, MAX_GAME_LENGTH> history;
    int history_cnt;
    int uci_moves_cnt;

    // original positions of rooks that are available for castling
    std::array<std::array<Bitboard, CS_NR>, COLOR_NR> origin_rooks;

    std::array<Piece, SQUARE_NR> board;
    std::array<Bitboard, COLOR_NR> color_bbs;
    std::array<Bitboard, PIECE_TYPE_NR + 1> type_bbs;
#pragma endregion
};

#endif //MEETRA_BOARD_H
