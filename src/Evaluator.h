#ifndef MEETRA_EVALUATOR_H
#define MEETRA_EVALUATOR_H

#include "Types.h"

namespace Meetra {

#define POSITIVE_INF 32000
#define NEGATIVE_INF (-32000)
#define MATE_SCORE 31000
#define DRAW_SCORE 0

    class Board;

    namespace Evaluation {

        class Evaluator {

            int mg[COLOR_NR];
            int eg[COLOR_NR];
            int phase;

            int mg_score;
            int eg_score;
            int mg_phase;
            int eg_phase;

        public:
            void SetBoard(const Board &board);
            void MakeMove(const Board &board, Move m);
            [[nodiscard]] Score GetMoveEval(const Board &board, Move m) const;
            [[nodiscard]] Score GetBoardEval() const;
        };

    }
}

#endif //MEETRA_EVALUATOR_H
