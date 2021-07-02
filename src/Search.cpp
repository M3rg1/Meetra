#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>
#include "Timer.h"

namespace Meetra {

    volatile bool run;
    Move best_move;
    int best_score;
    ulong nodes_searched;
    ulong qsearch_nodes;
    int qsearch_depth;
    int curr_depth;
    long timer_start;
    bool mate_found;
    Timer timer;

    void StopSearch() {
        run = false;
    }

    bool IsSearching() {
        return run;
    }

    void InitSearch() {
        using namespace std::chrono;
        mate_found = false;
        best_move = INVALID_MOVE;
        best_score = NEGATIVE_INF;
        nodes_searched = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_depth = 0;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        run = true;
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

    void SendBestMove() {
        std::cout << "bestmove " << GetMoveName(best_move) << std::endl;
    }

    void SendInfo() {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        elapsed_ms = std::max(1l, elapsed_ms);
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        // do it in 1 cout, this is baaad with so many cout accesses
        std::cout << "info " << "depth " << curr_depth << " nodes " << (nodes_searched + qsearch_nodes) << " time "
                  << elapsed_ms << " nps " << nps << " pv " << GetMoveName(best_move);
        if (mate_found) {
            // just do board.ColorTomove ? WHITE : *-1 : *1 instead of this ugly if
            if (best_score == MATE_SCORE) {
                std::cout << " score mate " << (curr_depth + 1) / 2;
            } else {
                std::cout << " score mate " << -(curr_depth + 1) / 2;
            }
        } else {
            std::cout << " score cp " << best_score;
        }
        std::cout << std::endl;
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
            //return BoardEval(board);
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
            } else if (!pv_move) {
                pv_move = move;
            }
        }

        if (pv_move == INVALID_MOVE) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE;
            }
            return DRAW_SCORE;
        }

        return alpha;
    }

    void StartSearch(Board board, int max_depth, long allowed_time) {

        if(allowed_time != INFINITE_TIMER){
            timer.SetTimeout(StopSearch, allowed_time);
        }

        for (curr_depth = 1; curr_depth <= max_depth; curr_depth++) {

            MoveGen move_gen(board);
            Move move;
            int best_score_this_iter = NEGATIVE_INF;
            Move pv_move = INVALID_MOVE;

            while ((move = move_gen.GetNextMove<false>())) {
                if (!board.MakeMove(move)) {
                    board.UnmakeMove(move);
                    continue;
                }
                nodes_searched++;
                auto score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, curr_depth);
                board.UnmakeMove(move);
                if (score > best_score_this_iter) {
                    best_score_this_iter = score;
                    pv_move = move;
                }
            }

            if (run) {
                best_move = pv_move;
                best_score = best_score_this_iter;
                if (std::abs(best_score) == MATE_SCORE) {
                    // should choose the mating sequence that is longest
                    // now it just goes with whatever move it tried first, because all other moves lead to mate
                    // as well, so it doesnt update the best_score_this_iter
                    // i think this might be possible to avoid when we will choose the principal variation move first
                    // so it automatically chooses the best move (the one that took the longest to find the mating patter for) first
                    mate_found = true;
                }
            }

            // TODO have sendinfo on another thread on timer (send it to the threadpool as repeated task every x seconds)
            // or even better, have that other thread Thread_Pool infomation from this thread every second or so
            // so we dont even need this if and sendinfo - we can just do a final sendinfo from the SendBestMove
            // method, and also turn off the auto sending thread there
            if (pv_move) {
                SendInfo();
            }

            if (!run || (allowed_time != INFINITE_TIMER && NotEnoughTimeLeft(allowed_time)) || mate_found ||
                !pv_move) {
                break;
            }
        }
        SendBestMove();
        run = false;
        timer.Stop();
    }
}
