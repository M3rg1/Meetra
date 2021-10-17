#ifndef MEETRA_EVALUATOR_H
#define MEETRA_EVALUATOR_H

#include "Defs.h"

class Board;

namespace Evaluation {

    class Evaluator {

        Score mg[COLOR_NR];
        Score eg[COLOR_NR];
        int phase;

        Score mg_score;
        Score eg_score;
        int mg_phase;
        int eg_phase;

    public:
        void SetBoard(const Board &board);
        void MakeMove(const Board &board, Move m);
        [[nodiscard]] Score GetMoveEval(const Board &board, Move m) const;
        [[nodiscard]] Score GetBoardEval() const;
        [[nodiscard]] inline int GetPhase() const { return phase; }
    };

}

#endif //MEETRA_EVALUATOR_H
