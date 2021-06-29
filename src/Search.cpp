#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>

namespace Meetra {

    ulong cutoffs = 0;
    ulong nodes_searched = 0;
    ulong qsearch_nodes = 0;
    ulong qsearch_depth = 0;

    int QuiescenceSearch(Board &board, int alpha, int beta, int depth) {

        if (depth > qsearch_depth) {
            qsearch_depth = depth;
        }

        auto score = BoardEval(board);
        if (score >= beta) {
            cutoffs++;
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }

        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            qsearch_nodes++;
            score = -QuiescenceSearch(board, -beta, -alpha, depth + 1);
            board.UnmakeMove(move);
            if (score >= beta) {
                cutoffs++;
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    int NegaMax(Board &board, int alpha, int beta, int depth) {


        if (depth == 0) {
            //return BoardEval(board);
            return QuiescenceSearch(board, alpha, beta, 1);
        }

        if (board.Ply() >= 50) {
            return DRAW_SCORE;
        }

        MoveGen move_gen(board);
        Move best_move = INVALID_MOVE;
        Move move;
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_searched++;
            auto score = -NegaMax(board, -beta, -alpha, depth - 1);
            board.UnmakeMove(move);
            if (score >= beta) {
                cutoffs++;
                return beta;
            }
            if (score > alpha) {
                alpha = score;
                best_move = move;
            }
        }

        if (best_move == INVALID_MOVE) {
            if (move_gen.IsKingInCheck()) {
                return MATE_SCORE;
            }
            return DRAW_SCORE;
        }

        return alpha;
    }

    void StartSearch(Board &board, int max_depth) {

        Move best_move = INVALID_MOVE;
        int best_score = NEGATIVE_INF;
        nodes_searched = 0;
        qsearch_nodes = 0;
        cutoffs = 0;

        for (int i = 1; i <= max_depth; i++) {
            MoveGen move_gen(board);
            Move move;
            while ((move = move_gen.GetNextMove<false>())) {
                if (!board.MakeMove(move)) {
                    board.UnmakeMove(move);
                    continue;
                }
                nodes_searched++;
                auto score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, i - 1);
                board.UnmakeMove(move);
                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                }
            }
        }

        std::cout << "Normal nodes: " << nodes_searched << std::endl;
        std::cout << "Qsearch nodes: " << qsearch_nodes << std::endl;
        std::cout << "Best move: " << GetMoveName(best_move) << std::endl;
        std::cout << "Score: " << best_score << std::endl;
        std::cout << "Cutoffs: " << cutoffs << std::endl;
        std::cout << "Qsearch depth: " << qsearch_depth << std::endl;
    }
}
