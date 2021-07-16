#ifndef MEETRA_EVALUATION_H
#define MEETRA_EVALUATION_H

#include "Board.h"
#include "Misc.h"

namespace Meetra::Evaluation{

#define POSITIVE_INF Score(32000)
#define NEGATIVE_INF Score(-32000)
#define MATE_SCORE Score(31000)
#define DRAW_SCORE Score(0)
#define MIN_MATE_EVAL Score(MATE_SCORE - MAX_SEARCH_DEPTH)

    Score MoveEval(const Board &board, Move move);
    Score MoveMaterialEval(const Board &board, Move move);
    Score MovePositionEval(const Board &board, Move move);
    Score MoveCastlingEval(const Board &board, Move move);

    Score BoardEval(const Board &board);
    Score BoardMaterialEval(const Board &board);
    Score BoardPositionEval(const Board &board);

}

#endif //MEETRA_EVALUATION_H
