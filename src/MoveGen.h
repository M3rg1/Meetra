#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Types.h"
#include "Board.h"

namespace Meetra {

    enum GenPhase {
        CAPTURE, QUIET, END, DOUBLE_CHECK
    };

    enum GenType {
        QSEARCH, NORMAL
    };

    class MoveGen {

    public:

        explicit MoveGen(const Board &board);

        inline void PutTTMove(Move tt_move) {
            move_eval[moves_cnt].move = tt_move;
            move_eval[moves_cnt++].score = 10000;
        }

        template<GenType Type>
        [[nodiscard]] Move GetBestMove();
        [[nodiscard]] Move GetAnyMove();
        [[nodiscard]] bool IsPseudoLegal(Move m) const;
        [[nodiscard]] inline bool IsInCheck() const { return checkers; }

    private:

        struct ScoredMove {
            Move move;
            Score score;

            std::strong_ordering operator<=>(const ScoredMove &other) const {
                return score <=> other.score;
            }
        };

        const Board &board;

        GenPhase gen_phase;

        ScoredMove move_eval[MAX_LEGAL_MOVES];
        size_t moves_cnt;

        Bitboard checkers;
        Bitboard blockers;
        Bitboard legal_moves;
        Bitboard phase_mask;
        Bitboard enemy_pieces;
        Bitboard all_pieces;
        Bitboard empty_squares;
        Square king_s;
        Square ep_s;

        Color my_color;
        Color enemy_color;
        bool double_check;

        [[nodiscard]] inline bool Empty() const { return moves_cnt == 0; }
        inline Move PopMove() { return move_eval[--moves_cnt].move; }
        inline Move PopRef(ScoredMove &it) {
            std::swap(it, move_eval[--moves_cnt]);
            return move_eval[moves_cnt].move;
        }
        inline void PutMove(Move m) { move_eval[moves_cnt++].move = m; }
        inline void PutPromMoves(Square from, Square to) {
            PutMove(NewMove(from, to, PROMOTE_QUEEN));
            PutMove(NewMove(from, to, PROMOTE_ROOK));
            PutMove(NewMove(from, to, PROMOTE_BISHOP));
            PutMove(NewMove(from, to, PROMOTE_KNIGHT));
        }

        Move PickBestMove();
        void EvalMoves();

        template<Color C, GenType Type>
        void NextPhase();

        template<GenPhase phase, Color C>
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
