#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Defs.h"
#include "Board.h"
#include "Config.h"
#include <array>

class MoveGen {

public:

    explicit MoveGen(const Board &b);
    MoveGen(const Board &b, const std::array<Move, KILLER_SLOTS> &killer_moves);

    inline void PutTTMove(Move tt_move) {
        move_eval[moves_cnt].move = tt_move;
        move_eval[moves_cnt++].score = TT_EVAL_BONUS;
    }

    template<GenType T>
    [[nodiscard]] Move NextBestMove();
    [[nodiscard]] Move NextMove();
    [[nodiscard]] bool IsPseudoLegal(Move m);

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

    std::array<Move, KILLER_SLOTS> killers;
    std::array<ScoredMove, MAX_LEGAL_MOVES> move_eval;
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

    template<PieceType PT, Color C, GenMode M>
    auto MovesForPT(Bitboard pieces, Bitboard legality_mask, Move to_validate = {});

    template<Color C, GenMode M>
    auto EpMoves(Bitboard pieces, Move to_validate = {});

    template<Color C, PawnMoveDir D, GenMode M>
    auto PawnProms(Bitboard pieces, Move to_validate = {});

    template<Color C, PawnMoveDir D, GenMode M>
    auto PawnCaptures(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M>
    auto PawnOneFwd(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M>
    auto PawnTwoFwd(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M>
    auto CastlingMoves(Move to_validate = {});

    template<Color C, CastlingSide S>
    [[nodiscard]] bool CanCastle() const;
    [[nodiscard]] bool DiscoveryCheck(Square origin, Square destination) const;

    template<Color C>
    [[nodiscard]] bool SpecialMoveIsPseudoLegal(Move m);
    template<Color C>
    [[nodiscard]] bool NormalMoveIsPseudoLegal(Move m);
};

#endif //MEETRA_MOVEGEN_H
