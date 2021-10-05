#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Defs.h"
#include "Board.h"
#include "Config.h"

namespace Meetra {

    class MoveGen {

    public:

        enum GenType {
            QSEARCH, NORMAL
        };

        explicit MoveGen(const Board &board);

        inline void PutTTMove(Move tt_move) {
            move_eval[moves_cnt].move = tt_move;
            move_eval[moves_cnt++].score = 10000;
        }

        template<GenType T>
        [[nodiscard]] Move GetBestMove();
        [[nodiscard]] Move GetAnyMove();
        [[nodiscard]] bool IsPseudoLegal(Move m) const;
        [[nodiscard]] inline bool IsCapture(Move m) const { return board.GetPieceOnSquare(ToSquare(m)) != NO_PIECE; }
        [[nodiscard]] inline bool IsInCheck() const { return checkers; }

    private:

        enum GenPhase {
            CAPTURE, QUIET, END, DOUBLE_CHECK
        };

        struct ScoredMove {
            Move move;
            Score score;

            std::strong_ordering operator<=>(const ScoredMove &other) const {
                return score <=> other.score;
            }
        };

        const Board &board;

        Color my_color;
        Color enemy_color;

        Square king_s;
        Square ep_s;
        Bitboard checkers;
        Bitboard blockers;
        Bitboard legal_moves;
        Bitboard all_pieces;
        Bitboard enemy_pieces;
        Bitboard empty_squares;
        bool double_check;

        GenPhase gen_phase;
        Bitboard phase_mask;

        ScoredMove move_eval[MAX_LEGAL_MOVES];
        int moves_cnt;

        [[nodiscard]] inline bool Empty() const { return moves_cnt == 0; }
        inline Move PopBack() { return move_eval[--moves_cnt].move; }
        inline Move PopRef(ScoredMove &it) {
            moves_cnt--;
            std::swap(it, move_eval[moves_cnt]);
            return move_eval[moves_cnt].move;
        }
        inline void PutMove(Move m) { move_eval[moves_cnt++].move = m; }
        inline void PutPromMoves(Move m) {
            PutMove(m | PROMOTE_QUEEN);
            PutMove(m | PROMOTE_ROOK);
            PutMove(m | PROMOTE_BISHOP);
            PutMove(m | PROMOTE_KNIGHT);
        }
        inline void EvalMoves() {
            for (int i = 0; i < moves_cnt; i++) {
                move_eval[i].score = board.GetMoveEval(move_eval[i].move);
            }
        }

        template<Color C, GenType T>
        void NextPhase();

        template<GenPhase P, Color C>
        void GenMovesForPhase();

        template<PieceType PT, Color C>
        void GenMovesForPieceType(Bitboard legality_mask);

        template<Color C>
        void GenEpMoves();

        template<Color C, PawnMoveDir D>
        void GenPawnCaptures();

        template<Color C>
        void GenPawnQuiets();

        template<Color C>
        void GenCastlingMoves();

        template<Color C, CastlingSide S>
        [[nodiscard]] bool CanCastle() const;

        template<Color C>
        [[nodiscard]] bool ValidateCastling(Move m) const;

        [[nodiscard]] bool DiscoveryCheck(Square origin, Square destination) const;
    };

}

#endif //MEETRA_MOVEGEN_H
