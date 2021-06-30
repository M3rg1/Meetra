#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>

namespace Meetra {

    volatile bool run;
    Move best_move;
    int best_score;
    ulong nodes_searched;
    ulong qsearch_nodes ;
    ulong qsearch_depth;

    void StopSearch(){
        run = false;
    }

    bool IsSearching(){
        return run;
    }

    int QuiescenceSearch(Board &board, int alpha, int beta, int depth) {

        if (depth > qsearch_depth) {
            qsearch_depth = depth;
        }

        auto score = BoardEval(board);
        if (score >= beta) {
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
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    int NegaMax(Board &board, int alpha, int beta, int depth) {

        if(!run){
            return 0;
        }

        if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 1);
        }

        if (board.Ply() >= 50) {
            return DRAW_SCORE;
        }

        MoveGen move_gen(board);
        Move pv_move = INVALID_MOVE;
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
                return beta;
            }
            if (score > alpha) {
                alpha = score;
                pv_move = move;
            }
        }

        if (pv_move == INVALID_MOVE) {
            if (move_gen.IsKingInCheck()) {
                return MATE_SCORE;
            }
            return DRAW_SCORE;
        }

        return alpha;
    }

    void SendInfo(){
        std::cout << "Normal nodes: " << nodes_searched << std::endl;
        std::cout << "Qsearch nodes: " << qsearch_nodes << std::endl;
        std::cout << "Best move: " << GetMoveName(best_move) << std::endl;
        std::cout << "Score: " << best_score << std::endl;
        std::cout << "Qsearch depth: " << qsearch_depth << std::endl;
        std::cout << std::endl << std::endl;
    }

    void StartSearch(Board board, int max_depth) {

        run = true;
        best_move = INVALID_MOVE;
        best_score = NEGATIVE_INF;
        nodes_searched = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;

        for (int i = 1; i <= max_depth && run; i++) {
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
            SendInfo();
        }
        run = false;
    }
}
