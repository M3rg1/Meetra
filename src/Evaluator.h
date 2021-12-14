#ifndef MEETRA_EVALUATOR_H
#define MEETRA_EVALUATOR_H

#include "Defs.h"
#include <array>

class Board;

class Evaluator {

public:

    void SetBoard(const Board &board);
    void MakeMove(const Board &board, Move m);
    [[nodiscard]] Score GetMoveEval(const Board &board, Move m) const;
    [[nodiscard]] Score GetBoardEval() const;
    [[nodiscard]] inline int GetPhase() const { return phase; }

private:

    std::array<Score, COLOR_NR> mg;
    std::array<Score, COLOR_NR> eg;
    int phase;

    Score mg_score;
    Score eg_score;
    int mg_phase;
    int eg_phase;
};

#endif //MEETRA_EVALUATOR_H
