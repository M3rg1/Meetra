#ifndef MEETRA_EVALUATION_H
#define MEETRA_EVALUATION_H

#include "Board.h"

namespace Meetra::Evaluation{

#define POSITIVE_INF 32000
#define NEGATIVE_INF (-32000)
#define MATE_SCORE 31000
#define DRAW_SCORE 0
#define MAX_SEARCH_THREADS 8
#define MAX_SEARCH_DEPTH 128
#define MIN_MATE_EVAL (MATE_SCORE - MAX_SEARCH_DEPTH)


    [[nodiscard]] Score MoveEval(const Board &board, Move move);
    [[nodiscard]] Score MoveMaterialEval(const Board &board, Move move);
    [[nodiscard]] Score MovePositionEval(const Board &board, Move move);
    [[nodiscard]] Score MoveCastlingEval(const Board &board, Move move);

    [[nodiscard]] Score BoardEval(const Board &board);
    [[nodiscard]] Score BoardMaterialEval(const Board &board);
    [[nodiscard]] Score BoardPositionEval(const Board &board);

}

#endif //MEETRA_EVALUATION_H
