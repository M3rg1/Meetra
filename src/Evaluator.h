#ifndef MEETRA_EVALUATOR_H
#define MEETRA_EVALUATOR_H

#include "Defs.h"
#include <array>
#include <vector>
#include <cstdio>
#include <tensorflow/c/c_api.h>
#include <string>
#include <complex>
#include "include/cppflow/model.h"
#include "include/cppflow/ops.h"

class Board;

class Evaluator {

public:

    void SetBoard(const Board &board);
    void MakeMove(const Board &board, Move m);
    [[nodiscard]] Score GetMoveEval(const Board &board, Move m) const;
    [[nodiscard]] Score GetBoardEval(const Board &b);
    [[nodiscard]] inline int GetPhase() const { return phase; }

private:
    [[nodiscard]] std::vector<uint8_t> GetValues(const Board &b) const;
    std::array<Score, COLOR_NR> mg;
    std::array<Score, COLOR_NR> eg;
    int phase;
    static cppflow::model model;
    Score mg_score;
    Score eg_score;
    int mg_phase;
    int eg_phase;
};

#endif //MEETRA_EVALUATOR_H
