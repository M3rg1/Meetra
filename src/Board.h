#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Types.h"
#include "ZobristHash.h"

namespace Meetra {

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_LEGAL_MOVES 256
#define MAX_GAME_LENGTH 512 // More than 511 moves won't fit in game state

    class Board {

    public:

        Board();
        bool NewPosition(const std::string &fen);
        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        bool MakeUciMove(const std::string &move_string);
        bool IsMoveLegal(Move m);
        [[nodiscard]] bool IsBoardValid() const;
        [[nodiscard]] bool IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard SquareAttackers(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard PinnedPiecesForSquare(Square s, Color blockers_color) const;

#pragma region ===== Getters =====
        [[nodiscard]] inline int HistorySize() const { return static_cast<int>(history_cnt); }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetEmptySquares() const { return ~GetPieces(ALL_TYPES); }
        [[nodiscard]] inline Piece GetPieceOnSquare(Square s) const { return board[s]; }
        [[nodiscard]] inline PieceType GetPieceTypeOnSq(Square s) const { return TypeOfPiece(GetPieceOnSquare(s)); }
#pragma endregion

#pragma region ===== Game State info getters =====
        [[nodiscard]] inline CastlingRights GetCR() const { return static_cast<CastlingRights>(curr_data.state & ALL_CR); }
        [[nodiscard]] inline ZobristHash GetZobristHash() const { return curr_data.hash; }
        [[nodiscard]] inline bool CanWhiteShortCR() const { return curr_data.state & WHITE_SHORT; }
        [[nodiscard]] inline bool CanWhiteLongCR() const { return curr_data.state & WHITE_LONG; }
        [[nodiscard]] inline bool CanBlackShortCR() const { return curr_data.state & BLACK_SHORT; }
        [[nodiscard]] inline bool CanBlackLongCR() const { return curr_data.state & BLACK_LONG; }
        [[nodiscard]] inline bool CanCastleAny() const { return curr_data.state & ALL_CR; }
        [[nodiscard]] inline Square EpSquare() const { return static_cast<Square >(curr_data.state & 0x3F); }
        [[nodiscard]] inline Color ColorToMove() const { return static_cast<Color>((curr_data.state >> 10) & 0x1); }
        [[nodiscard]] inline Piece CapturedPiece() const { return static_cast<Piece>((curr_data.state >> 11) & 0xF); }
        [[nodiscard]] inline int Ply() const { return static_cast<int>((curr_data.state >> 15) & 0xFF); }
        [[nodiscard]] inline int TotalMoves() const { return static_cast<int>(curr_data.state >> 23); }
        [[nodiscard]] inline bool Move50Rule() const { return Ply() >= 100; };
        [[nodiscard]] inline bool IsRepetition() const {
            // http://www.talkchess.com/forum3/viewtopic.php?f=7&t=51000&start=20
            // should implement draw at 1st repetition in the search tree, 2nd in actual game history
            // because moves played before could be sub-optimal, however moves during the search have already been
            // examined, so repeating even once is pointless
            int rep = 0;
            int stop = std::max(HistorySize() - Ply(), 0);
            for (int i = HistorySize() - 2; i >= stop; i -= 2) {
                if (history[i].hash == curr_data.hash) {
                    if (++rep > 1) {
                        return true;
                    }
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
        // bits 15-22 = ply since last capture/pawn moves - 50 move rule
        // bits 23+ - total moves made
        // TODO can have one bit flag for endgame - is that even worth doing now? check some eval techniques first
        typedef uint32_t GameState;
#define NEW_GAME_STATE 0
#pragma endregion

#pragma region ===== Game State modifications =====
        // requires new game state
        inline void SetEpSquare(Square s) { curr_data.state |= static_cast<GameState>(s); }
        inline void SetCastlingRights(CastlingRights cr) { curr_data.state |= static_cast<GameState>(cr); }
        inline void SetColorToMove(Color c) { curr_data.state |= static_cast<GameState>(c << 10); }
        inline void SetCapturedPiece(Piece p) { curr_data.state |= static_cast<GameState>(p << 11); }
        inline void SetPly(int ply) { curr_data.state |= static_cast<GameState>(ply << 15); }
        inline void SetMoveNumber(int move_num) { curr_data.state |= static_cast<GameState>(move_num << 23); }

        // modify current game state
        inline void ResetPly() { curr_data.state &= static_cast<GameState>(~(0xFF << 15)); }
        inline void RemoveCastlingRights(CastlingRights cr) { curr_data.state &= static_cast<GameState>(~cr); }
        inline void IncrementMoveNumber(Color col_to_move) { curr_data.state += col_to_move << 23; }
        inline void IncrementPly() { curr_data.state += 1 << 15; }
        inline void ClearCapturedPiece() { curr_data.state &= static_cast<GameState>(~0x7800); }
        inline void ChangeColorToMove() { curr_data.state ^= 1 << 10; }
        inline void ClearEpSquare() { curr_data.state &= static_cast<GameState>(~0x3F); }
#pragma endregion

#pragma region ===== Update inner structures =====
        void ParseFen(const std::string &fen);
        bool ParseFenValidate(const std::string &fen);
        void RemovePiece(Square s);
        void PutPiece(Square s, Piece p);
        void MovePiece(Square from, Square to);
#pragma endregion

#pragma region ===== Data =====
        struct BoardData {
            GameState state;
            ZobristHash hash;
        };

        BoardData history[MAX_GAME_LENGTH];
        size_t history_cnt;

        BoardData curr_data;
        Piece board[SQUARE_NR];
        Bitboard color_bbs[COLOR_NR];
        Bitboard type_bbs[PIECE_TYPE_NR];
#pragma endregion
    };
}


#endif //MEETRA_BOARD_H
