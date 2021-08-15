#ifndef MEETRA_BOARD_H
#define MEETRA_BOARD_H

#include "Types.h"
#include "ZobristHash.h"

namespace Meetra {

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_LEGAL_MOVES 128
#define MAX_GAME_LENGTH 512

    class Board {

    public:
        Board();
        void NewPosition(const std::string &fen);
        bool MakeMove(Move m);
        void UnmakeMove(Move m);
        [[nodiscard]] bool IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard SquareAttackers(Square s, Color attacked_by, Bitboard occ) const;
        [[nodiscard]] Bitboard PinnedPiecesForSquare(Square s, Color blockers_color) const;
        [[nodiscard]] int MovesMadeCount() const { return history_cnt; }

#pragma region ===== Piece getters =====
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt, Color c) const { return type_bbs[pt] & color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetPieces(PieceType pt) const { return type_bbs[pt]; }
        [[nodiscard]] inline Bitboard GetPieces(Color c) const { return color_bbs[c]; }
        [[nodiscard]] inline Bitboard GetEmptySquares() const { return ~GetPieces(ALL_TYPES); }
        [[nodiscard]] inline PieceType GetPieceTypeOnSq(Square s) const { return TypeOfPiece(board[s]); }
#pragma endregion

#pragma region ===== Game State info getters =====
        [[nodiscard]] inline ZobristHash GetZobristHash() const { return curr_data.hash; }
        [[nodiscard]] inline CastlingRights GetCR() const { return static_cast<CastlingRights>(curr_data.state & ALL_CR); }
        [[nodiscard]] inline bool CanWhiteShortCR() const { return (curr_data.state & WHITE_SHORT) != 0; }
        [[nodiscard]] inline bool CanWhiteLongCR() const { return (curr_data.state & WHITE_LONG) != 0; }
        [[nodiscard]] inline bool CanBlackShortCR() const { return (curr_data.state & BLACK_SHORT) != 0; }
        [[nodiscard]] inline bool CanBlackLongCR() const { return (curr_data.state & BLACK_LONG) != 0; }
        [[nodiscard]] inline bool CanColorCastleAny(Color c) const {
            return c == WHITE ? curr_data.state & WHITE_ALL_CR : curr_data.state & BLACK_ALL_CR;
        }
        [[nodiscard]] inline bool CanCastleAny() const { return curr_data.state & ALL_CR; }
        [[nodiscard]] inline Square EpSquare() const { return static_cast<Square >(curr_data.state & 0x3F); }
        [[nodiscard]] inline Color ColorToMove() const { return static_cast<Color>(curr_data.state >> 10 & 0x1); }
        [[nodiscard]] inline Piece CapturedPiece() const { return static_cast<Piece>(curr_data.state >> 11 & 0xF); }
        [[nodiscard]] inline int Ply() const { return static_cast<int>(curr_data.state >> 15 & 0x3F); }
        [[nodiscard]] inline int TotalMoves() const { return static_cast<int>(curr_data.state >> 22); }
        [[nodiscard]] inline bool IsRepetition() const {
            // http://www.talkchess.com/forum3/viewtopic.php?f=7&t=51000&start=20
            // should implement draw at 1st repetition in the search tree, 2nd in actual game history
            // because moves played before could be sub-optimal, however moves during the search have already been
            // examined, so repeating even once is pointless
            for (int i = history_cnt - 2; i >= 0; i -= 2) {
                if (history[i].state & 0x7800) {
                    return false;
                } else if (history[i].hash == curr_data.hash) {
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
        inline void SetEpSquare(Square s) { curr_data.state |= static_cast<GameState>(s); }
        inline void SetCastlingRights(CastlingRights cr) { curr_data.state |= static_cast<GameState>(cr); }
        inline void SetColorToMove(Color c) { curr_data.state |= static_cast<GameState>(c << 10); }
        inline void SetCapturedPiece(Piece p) { curr_data.state |= static_cast<GameState>(p << 11); }
        inline void SetPly(int ply) { curr_data.state |= static_cast<GameState>(ply << 15); }
        inline void SetMoveNumber(int move_num) { curr_data.state |= static_cast<GameState>(move_num << 22); }

        // modify current game state
        inline void ResetPly() { curr_data.state &= static_cast<GameState>(~0x3F8000); }
        inline void RemoveCastlingRights(CastlingRights cr) { curr_data.state &= static_cast<GameState>(~cr); }
        inline void IncrementMoveNumber(Color col_to_move) { curr_data.state += col_to_move << 22; }
        inline void IncrementPly() { curr_data.state += 1 << 15; }
        inline void ClearCapturedPiece() { curr_data.state &= static_cast<GameState>(~0x7800); }
        inline void ChangeColorToMove() { curr_data.state ^= 1 << 10; }
        inline void ClearEpSquare() { curr_data.state &= static_cast<GameState>(~0x3F); }
#pragma endregion

#pragma region ===== Update inner structures =====
        void ParseFen(const std::string &fen);
        void RemovePiece(Square s, ZobristHash &h);
        void PutPiece(Square s, Piece p, ZobristHash &h);
        void MovePiece(Square from, Square to, ZobristHash &h);
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
        int history_cnt;

        BoardData curr_data;
        Piece board[SQUARE_NR];
        Bitboard color_bbs[COLOR_NR];
        Bitboard type_bbs[PIECE_TYPE_NR];
#pragma endregion
    };
}


#endif //MEETRA_BOARD_H
