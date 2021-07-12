#ifndef MEETRA_EVALUATION_H
#define MEETRA_EVALUATION_H

#include "Board.h"

namespace Meetra{

#define POSITIVE_INF 32000
#define NEGATIVE_INF (-32000)
#define MATE_SCORE 31000
#define DRAW_SCORE 0

    inline bool IsScoreMate(Score score){
        return std::abs(score) >= MATE_SCORE - MAX_SEARCH_DEPTH;
    }

    Score MoveEval(const Board &board, Move move);
    Score MoveMaterialEval(const Board &board, Move move);
    Score MovePositionEval(const Board &board, Move move);
    Score MoveCastlingEval(const Board &board, Move move);

    Score BoardEval(const Board &board);
    Score BoardMaterialEval(const Board &board);
    Score BoardPositionEval(const Board &board);

}

#endif //MEETRA_EVALUATION_H
