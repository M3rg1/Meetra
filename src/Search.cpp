#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "ThreadPool.h"
#include "UciHandler.h"
#include <execution>
#include "omp.h"

namespace Meetra {

#define MULTI_PV 1

    ABSearch::ABSearch() {
        run = false;
    }

    void ABSearch::InitSearch(Board &board) {
        tt.NewSearch();
        tt_hits = 0;
        normal_nodes = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_max_depth = 0;
        root_moves_cnt = 0;
        using namespace std::chrono;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        GenRootNodes(board);
        SortRootNodes();
    }

    // TODO fixed search time isnt working right now! we ending when 50% time remaining!!!
    //  the UciHandler will have to let us know this is fixed search time, so we dont exist early via NotEnoughTime foo
    void ABSearch::StartSearch(Board &board, Depth max_depth, long allowed_time, int num_threads) {

        run = true;
        InitSearch(board);
        omp_set_dynamic(0);
        omp_set_num_threads(std::min(MAX_SEARCH_THREADS, num_threads));

        if (allowed_time != INFINITE_TIMER) {
            search_timer.SetTimeout([&]() { StopSearch(); }, allowed_time);
        }
        info_timer.SetInterval([&]() {
            UciHandler::SendToGui(GetUpdateSearchInfo());
        }, 1000);

        max_depth = std::min(max_depth, MAX_SEARCH_DEPTH);
        for (curr_max_depth = 1; curr_max_depth <= max_depth; curr_max_depth++, qsearch_depth = 0) {

#pragma omp parallel for default(none) ordered schedule(static, 1) firstprivate(board)
            for (int curr_move_num = 0; curr_move_num < root_moves_cnt; curr_move_num++) {
                Move curr_move = root_moves[curr_move_num].move;

                if (run && ElapsedTimeMs() > 1000) {
                    UciHandler::SendToGui(GetCurrMoveInfo(curr_move, curr_move_num));
                }

                board.MakeMove(curr_move);
                normal_nodes++;
                Score score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, curr_max_depth - 1, 1);
                board.UnmakeMove(curr_move);
                if (!run) {
                    continue;
                }
                root_moves[curr_move_num].score = score;
            }

            SortRootNodes();
            UciHandler::SendToGui(GetSearchInfo(board));

            if (!run || !EnoughTimeLeft(allowed_time) || IsScoreMate(root_moves[0].score)) {
                break;
            }
        }

        run = false;
        search_timer.Stop();
        info_timer.Stop();
        UciHandler::SendToGui(GetBestMove());
    }

    void ABSearch::ExperimentalStartSearch(Board &board, Depth max_depth, long allowed_time, int num_threads) {

        run = true;
        InitSearch(board);
        omp_set_dynamic(0);
        omp_set_num_threads(std::min(MAX_SEARCH_THREADS, num_threads));

        if (allowed_time != INFINITE_TIMER) {
            search_timer.SetTimeout([&]() { StopSearch(); }, allowed_time);
        }
        info_timer.SetInterval([&]() {
            UciHandler::SendToGui(GetUpdateSearchInfo());
        }, 1000);

        max_depth = std::min(max_depth, MAX_SEARCH_DEPTH);
        qsearch_depth = 0;
        int top_depth = 0;

#pragma omp parallel for default(none) schedule(static, 1) firstprivate(board) shared(max_depth, allowed_time, top_depth)
        for (int search_depth = 1; search_depth <= max_depth; search_depth++) {

            if (!run || !EnoughTimeLeft(allowed_time)) {
                continue;
            }

            for (int curr_move_num = 0; curr_move_num < root_moves_cnt; curr_move_num++) {
                Move curr_move = root_moves[curr_move_num].move;
                board.MakeMove(curr_move);
                normal_nodes++;
                Score score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, search_depth - 1, 1);
                board.UnmakeMove(curr_move);
                if (!run) {
                    break;
                }
#pragma omp critical
                {
                    if (root_moves[curr_move_num].depth_searched < search_depth) {
                        root_moves[curr_move_num].score = score;
                        root_moves[curr_move_num].depth_searched = search_depth;
                    }

                }
            }
            if (run) {
#pragma omp critical
                {
                    if (search_depth > top_depth) {
                        top_depth = search_depth;
                    }
                    UciHandler::SendToGui(ExperimentalGetSearchInfo(board, search_depth));
                }
            }
        }
        SortRootNodes();
        run = false;
        search_timer.Stop();
        info_timer.Stop();
        UciHandler::SendToGui(ExperimentalGetSearchInfo(board, top_depth));
        UciHandler::SendToGui(GetBestMove());
    }

    std::string ABSearch::ExperimentalGetSearchInfo(Board &board, Depth depth) {
        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(normal_nodes + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        auto pvs_to_send = 1;
        Score top_score = NEGATIVE_INF;
        int i = 0;
        for (int idx = 0; i < root_moves_cnt; i++) {
            if (root_moves[idx].score > top_score) {
                top_score = root_moves[idx].score;
                i = idx;
            }
        }
        ss << "info multipv " << i + 1
           << " depth " << +depth
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull " << static_cast<int>(tt.Usage() * 1000)
           << " score ";

        int mate_length_ply = 0;
        if (root_moves[i].score >= MATE_SCORE - MAX_SEARCH_DEPTH) {
            mate_length_ply = static_cast<int>(MATE_SCORE - root_moves[i].score);
            ss << "mate " << mate_length_ply / 2;
        } else if (root_moves[i].score <= -MATE_SCORE + MAX_SEARCH_DEPTH) {
            mate_length_ply = static_cast<int>(MATE_SCORE + root_moves[i].score);
            ss << "mate " << -mate_length_ply / 2;
        } else {
            ss << "cp " << root_moves[i].score;
        }

        ss << " pv " << GetMoveName(root_moves[i].move);
        board.MakeMove(root_moves[i].move);
        // TODO try to remove the max depth guard when we have working repetition recognition
        RetrievePv(board, pv_stack, std::max(depth - 1, mate_length_ply));
        board.UnmakeMove(root_moves[i].move);
        Move *pv_stack_ptr = pv_stack;
        Move pv_move;
        while ((pv_move = *pv_stack_ptr++)) {
            ss << " " << GetMoveName(pv_move);
        }

        return ss.str();
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 0);
        }

        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
        if (score != NOT_FOUND) {
            tt_hits++;
            return score;
        }

        MoveGen move_gen(board, &tt);
        Move move;
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;
        bool no_legal_moves = true;

        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            normal_nodes++;
            score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1);
            board.UnmakeMove(move);
            if (!run) {
                return 0;
            } else if (score >= beta) {
                tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
                return beta;
            } else if (score > alpha) {
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move_this_iter = move;
            }
            no_legal_moves = false;
        }

        if (no_legal_moves) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply + 1;
            }
            return DRAW_SCORE;
        }

        tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

        return alpha;
    }

    Score ABSearch::QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth) {

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
            } else if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    bool ABSearch::EnoughTimeLeft(long allowed_time) const {
        if (allowed_time == INFINITE_TIMER || allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    void ABSearch::ResizeTT(TTSize size) {
        tt.Resize(size);
    }

    void ABSearch::ClearTT() {
        tt.Clear();
    }

    std::string ABSearch::GetBestMove() const {
        std::string ret = "bestmove ";
        ret.append(GetMoveName(root_moves[0].move));
        return ret;
    }

    long ABSearch::ElapsedTimeMs() const {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        return std::max(1l, elapsed_ms);
    }

    void ABSearch::RetrievePv(Board &board, Move *pv_line, Depth depth) {
        Move move = tt.GetPVMove(board.GetZobristHash());
        if (!move || depth == 0) {
            *pv_line = INVALID_MOVE;
            return;
        }
        *pv_line++ = move;
        board.MakeMove(move);
        RetrievePv(board, pv_line, depth - 1);
        board.UnmakeMove(move);
    }

    void ABSearch::GenRootNodes(Board &board) {
        MoveGen move_gen(board);
        Move m;
        while ((m = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            Score s = tt.ProbeEval(board.GetZobristHash(), NEGATIVE_INF, POSITIVE_INF, 0, 0);
            board.UnmakeMove(m);
            if (s == NOT_FOUND) {
                s = MoveEval(board, m);
            }
            root_moves[root_moves_cnt].move = m;
            root_moves[root_moves_cnt].score = s;
            root_moves[root_moves_cnt].depth_searched = 0;
            root_moves_cnt++;
        }
    }

    void ABSearch::SortRootNodes() {
        std::sort(std::execution::seq, root_moves, root_moves + root_moves_cnt,
                  [](const MoveAndEval &mae1, const MoveAndEval &mae2) {
                      return mae1.score > mae2.score;
                  });
    }

    std::string ABSearch::GetUpdateSearchInfo() const {

        auto elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(normal_nodes + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << +curr_max_depth
           << " seldepth " << qsearch_depth + curr_max_depth
           << " nodes " << (normal_nodes + qsearch_nodes)
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull " << static_cast<int>(tt.Usage() * 1000);

        return ss.str();
    }

    std::string ABSearch::GetSearchInfo(Board &board) {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(normal_nodes + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        //auto pvs_to_send = std::min(MULTI_PV, moves_count);
        auto pvs_to_send = 1;
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info multipv " << i + 1
               << " depth " << +curr_max_depth
               << " seldepth " << qsearch_depth + curr_max_depth
               << " nodes " << (normal_nodes + qsearch_nodes)
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score ";

            int mate_length_ply = 0;
            if (root_moves[i].score >= MATE_SCORE - MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE - root_moves[i].score);
                ss << "mate " << mate_length_ply / 2;
            } else if (root_moves[i].score <= -MATE_SCORE + MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE + root_moves[i].score);
                ss << "mate " << -mate_length_ply / 2;
            } else {
                ss << "cp " << root_moves[i].score;
            }

            ss << " pv " << GetMoveName(root_moves[i].move);
            board.MakeMove(root_moves[i].move);
            // TODO try to remove the max depth guard when we have working repetition recognition
            RetrievePv(board, pv_stack, std::max(curr_max_depth - 1, mate_length_ply));
            board.UnmakeMove(root_moves[i].move);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << " " << GetMoveName(pv_move);
            }
        }

        return ss.str();
    }

    std::string ABSearch::GetCurrMoveInfo(Move move, int num) const {
        std::string ret = "info currmove ";
        ret.append(GetMoveName(move));
        ret.append(" currmovenumber ");
        ret.append(std::to_string(num + 1));
        return ret;
    }
}
