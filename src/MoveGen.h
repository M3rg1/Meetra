#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Types.h"
#include "Board.h"
#include "TranspositionTable.h"

namespace Meetra {

    class MoveGen {

    public:
        MoveGen(const Board &board, const TranspositionTable *tt);
        explicit MoveGen(const Board &board);

        template<bool QSearch>
        Move GetNextMove();

        [[nodiscard]] inline bool IsKingInCheck() const { return checkers; }

    private:
        const Board &board;
        const TranspositionTable *tt;

        GenPhase genPhase;

        MoveAndEval move_eval[MAX_LEGAL_MOVES];
        size_t moves_cnt;

        Bitboard checkers;
        Bitboard blockers;
        Bitboard legal_moves;
        Bitboard phase_mask;
        Bitboard enemy_pieces;
        Bitboard all_pieces;
        Bitboard empty_squares;
        Square king_square;

        Color my_color;
        Color enemy_color;

        [[nodiscard]] inline bool Empty() const { return moves_cnt == 0; }
        inline Move PopMove() { return move_eval[--moves_cnt].move; }
        inline Move PopAtIdx(size_t idx) {
            Move ret = move_eval[idx].move;
            move_eval[idx] = move_eval[--moves_cnt];
            return ret;
        }
        inline Move PopRef(MoveAndEval &it) {
            Move ret = it.move;
            it = move_eval[--moves_cnt];
            return ret;
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

        template<Color C, bool QSearch>
        void NextPhase();

        template<GenPhase phase, Color C>
        void GenMovesForPhase();

        template<PieceType PT, Color C>
        void GenMovesForPieceType(Bitboard legality_mask);

        template<Color C>
        void GenEnPassantMoves();

        template<Color C>
        void GenPawnCaptures();

        template<Color C>
        void GenPawnForwardMoves();

        template<Color C>
        void GenCastlingMoves();

        template<Color C>
        [[nodiscard]] bool CanCastleLong(CastlingRights cr) const;
        template<Color C>
        [[nodiscard]] bool CanCastleShort(CastlingRights cr) const;
        [[nodiscard]] bool DiscoveryCheck(Square origin, Square destination) const;
    };

}

#endif //MEETRA_MOVEGEN_H
