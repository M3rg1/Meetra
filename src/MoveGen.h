#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Defs.h"
#include "Board.h"
#include "Config.h"

class MoveGen {

public:

    explicit MoveGen(const Board &board);
    MoveGen(const Board &board, const Move killer_moves[2]);

    inline void PutTTMove(Move tt_move) {
        move_eval[moves_cnt].move = tt_move;
        move_eval[moves_cnt++].score = 100000;
    }

    template<GenType T>
    [[nodiscard]] Move GetBestMove();
    [[nodiscard]] Move GetAnyMove();
    [[nodiscard]] bool IsPseudoLegal(Move m) const;

private:

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
    Bitboard all_pieces;
    Bitboard empty_squares;
    Bitboard enemy_pieces;
    Bitboard checkers;
    Bitboard blockers;
    Bitboard legal_moves;
    bool double_check;

    GenPhase gen_phase;
    Bitboard phase_mask;

    Move killers[2];
    ScoredMove move_eval[MAX_LEGAL_MOVES];
    int moves_cnt;

    [[nodiscard]] inline bool Empty() const { return moves_cnt == 0; }
    inline Move PopBack() { return move_eval[--moves_cnt].move; }
    inline Move PopRef(ScoredMove &it) {
        std::swap(it, move_eval[--moves_cnt]);
        return move_eval[moves_cnt].move;
    }
    inline void PutMove(Move m) { move_eval[moves_cnt++].move = m; }
    inline void PutPromMoves(Move m) {
        PutMove(m | PROMOTE_QUEEN);
        PutMove(m | PROMOTE_ROOK);
        PutMove(m | PROMOTE_BISHOP);
        PutMove(m | PROMOTE_KNIGHT);
    }

    void EvalMoves();

    template<Color C, GenType T>
    void NextPhase();

    template<GenPhase P, Color C>
    void GenMovesForPhase();

    template<PieceType PT, Color C>
    void GenMovesForPieceType(Bitboard legality_mask);

    template<Color C>
    void GenEpMoves();

    template<Color C, PawnMoveDir D>
    void GenPawnPromotions();

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

#endif //MEETRA_MOVEGEN_H
