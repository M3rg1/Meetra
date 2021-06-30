#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>

namespace Meetra {

    volatile bool run;
    Move best_move;
    int best_score;
    ulong nodes_searched;
    ulong qsearch_nodes;
    ulong qsearch_depth ;
    int curr_depth;
    long timer_start;

    void StopSearch() {
        run = false;
    }

    bool IsSearching() {
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

        // TODO in qsearch try not to order moves by position, just by victim/attacker .. maybe?
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

        if (!run) {
            return 0;
        }

        if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 1);
        }

        if (board.Ply() >= 50  /*|| repetition*/ ) {
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
            else if(!pv_move){
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

    void SendInfo() {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        elapsed_ms = std::max(1l, elapsed_ms);
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::cout << "info " << "depth " << curr_depth << " nodes " << (nodes_searched + qsearch_nodes) << " time "
                  << elapsed_ms << " nps " << nps << " score cp " << best_score << " pv " << GetMoveName(best_move)
                  << std::endl;
    }

    void SendBestMove() {
        std::cout << "bestmove " << GetMoveName(best_move) << std::endl;
    }

    bool NotEnoughTimeLeft(long allowed_time) {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed = now - timer_start;
        if (allowed_time < elapsed * 2) {
            return true;
        }
        return false;
    }

    void InitSearch() {
        using namespace std::chrono;
        best_move = INVALID_MOVE;
        best_score = NEGATIVE_INF;
        nodes_searched = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_depth = 0;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        run = true;
    }

    void StartSearch(Board board, int max_depth, long allowed_time) {

        for (curr_depth = 1; curr_depth <= max_depth && run; curr_depth++) {

            MoveGen move_gen(board);
            Move move;
            int best_score_this_iter = NEGATIVE_INF;
            Move best_move_this_iter = INVALID_MOVE;

            while ((move = move_gen.GetNextMove<false>())) {
                if (!board.MakeMove(move)) {
                    board.UnmakeMove(move);
                    continue;
                }
                nodes_searched++;
                auto score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, curr_depth - 1);
                board.UnmakeMove(move);
                if (score > best_score_this_iter) {
                    best_score_this_iter = score;
                    best_move_this_iter = move;
                }
            }

            if(run){
                best_move = best_move_this_iter;
                best_score = best_score_this_iter;
            }

            // TODO have sendinfo on another thread on timer (send it to the threadpool as repeated task every x seconds)
            SendInfo();

            // TODO or if score very high or if mate, just return
            if (allowed_time != INFINITE_TIMER && NotEnoughTimeLeft(allowed_time)) {
                break;
            }
        }
        SendBestMove();
        run = false;
    }
}
