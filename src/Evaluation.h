#ifndef MEETRA_EVALUATION_H
#define MEETRA_EVALUATION_H

#include "Board.h"

namespace Meetra{

#define POSITIVE_INF 32000
#define NEGATIVE_INF (-32000)
#define MATE_SCORE 31000
#define DRAW_SCORE 0

    int MoveEval(const Board &board, Move move);
    int MoveMaterialEval(const Board &board, Move move);
    int MovePositionEval(const Board &board, Move move);
    int MoveCastlingEval(const Board &board, Move move);

    int BoardEval(const Board &board);
    int BoardMaterialEval(const Board &board);
    int BoardPositionEval(const Board &board, Color c);

}

#endif //MEETRA_EVALUATION_H
