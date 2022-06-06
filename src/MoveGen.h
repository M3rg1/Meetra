#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include <array>
#include "Defs.h"
#include "Board.h"
#include "Config.h"

class MoveGen {

public:

    enum GenType {
        QSEARCH, NORMAL
    };

    explicit MoveGen(const Board &b, const std::array<Move, KILLER_SLOTS> &killer_moves = {});

    void PutTTMove(Move tt_move) {
        move_eval[moves_cnt++] = {tt_move, TT_EVAL_BONUS};
    }

    template<GenType T>
    [[nodiscard]] Move NextBestMove();
    [[nodiscard]] Move NextMove();
    [[nodiscard]] bool IsPseudoLegal(Move m);

private:

    enum GenMode {
        GENERATE, VALIDATE
    };

    enum GenPhase {
        PROMOTION, CAPTURE, QUIET, END, DOUBLE_CHECK
    };

    enum PawnMoveDir {
        LEFT, RIGHT, FORWARD
    };

    struct ScoredMove {
        Move move;
        Score score;

        constexpr std::strong_ordering operator<=>(const ScoredMove &other) const {
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
    Bitboard legal_moves = FULL_BB;
    bool double_check = false;

    GenPhase gen_phase = PROMOTION;

    std::array<Move, KILLER_SLOTS> killers;
    std::array<ScoredMove, MAX_LEGAL_MOVES> move_eval;
    int moves_cnt = 0;

    [[nodiscard]] bool Empty() const { return moves_cnt == 0; }
    Move PopBack() { return move_eval[--moves_cnt].move; }
    Move PopRef(ScoredMove &it) {
        std::swap(it, move_eval[--moves_cnt]);
        return move_eval[moves_cnt].move;
    }
    void PutMove(Move m) { move_eval[moves_cnt++].move = m; }
    void PutPromMoves(Move m) {
        PutMove(m | PROMOTE_QUEEN);
        PutMove(m | PROMOTE_ROOK);
        PutMove(m | PROMOTE_BISHOP);
        PutMove(m | PROMOTE_KNIGHT);
    }

    void EvalMoves();

    template<Color C, GenType T>
    void NextPhase();

    template<GenPhase P, Color C, GenType T>
    void GenMovesForPhase();

    template<PieceType PT, Color C, GenMode M = GENERATE>
    auto MovesForPT(Bitboard pieces, Bitboard legality_mask, Move to_validate = {});

    template<Color C, GenMode M = GENERATE>
    auto EpMoves(Bitboard pieces, Move to_validate = {});

    template<Color C, PawnMoveDir D, GenType T, GenMode M = GENERATE>
    auto PawnProms(Bitboard pieces, Move to_validate = {});

    template<Color C, PawnMoveDir D, GenMode M = GENERATE>
    auto PawnCaptures(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M = GENERATE>
    auto PawnOneFwd(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M = GENERATE>
    auto PawnTwoFwd(Bitboard pieces, Move to_validate = {});

    template<Color C, GenMode M = GENERATE>
    auto CastlingMoves(Move to_validate = {});

    template<Color C, CastlingSide S>
    [[nodiscard]] bool CanCastle() const;
    [[nodiscard]] bool DiscoveryCheck(Square origin, Square destination) const;

    template<Color C>
    [[nodiscard]] bool SpecialMoveIsPseudoLegal(Move m);
    template<Color C>
    [[nodiscard]] bool NormalMoveIsPseudoLegal(Move m);

    template<Color C, PawnMoveDir DIR>
    static constexpr Direction PawnMove() {
        return C == WHITE ? DIR == LEFT ? NORTH_WEST : DIR == RIGHT ? NORTH_EAST : NORTH
                          : DIR == LEFT ? SOUTH_EAST : DIR == RIGHT ? SOUTH_WEST : SOUTH;
    }
};

#endif //MEETRA_MOVEGEN_H
