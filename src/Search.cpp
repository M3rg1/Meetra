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


    ABSearch::ABSearch() {
        run = false;
        show_currline = false;
        multi_pv = 1;
        show_currmove = true;
        plies_muted = 1;
        omp_set_dynamic(0);
        omp_set_num_threads(DEFAULT_SEARCH_THREADS);
    }

    void ABSearch::InitSearch(SearchSettings &s) {

        settings = s;

        using namespace std::chrono;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();

        tt.NewSearch();
        normal_nodes = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_max_depth = 0;
        root_moves_cnt = 0;
        best_score = NEGATIVE_INF;
        best_move = INVALID_MOVE;

        GenRootNodes();
        SortRootNodes();
        settings.max_allowed_depth = std::min(settings.max_allowed_depth, MAX_SEARCH_DEPTH);

        InitSearchTimer();

        if (!settings.infinite) {
            search_timer.SetTimeout([&]() { StopSearch(); }, settings.allowed_time);
        }

        info_timer.SetInterval([&]() {
            UciHandler::SendToGui(GetUpdateSearchInfo());
        }, settings.info_to_ui_ms_timer);
    }


    void ABSearch::InitSearchTimer() {

        if (settings.infinite || settings.fixed_timer) {
            return;
        }

        auto time_left = settings.board.ColorToMove() == WHITE ? settings.white_time : settings.black_time;
        if (time_left) {
            int moves_made = std::min(settings.board.MovesMadeCount() + 1, 10);
            double factor = 2.0 - moves_made / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - moves_made;
            settings.allowed_time = static_cast<long>(factor * target);
        }
    }

    void ABSearch::StartSearch(SearchSettings s) {

        run = true;
        InitSearch(s);

        if (!root_moves_cnt) {
            run = false;
            UciHandler::SendToGui("bestmove 0000");
            return;
        }

        for (curr_max_depth = 2; curr_max_depth <= settings.max_allowed_depth; curr_max_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            qsearch_depth = 0;

            for (int curr_move_num = 0; curr_move_num < root_moves_cnt; curr_move_num++) {

                Move curr_move = root_moves[curr_move_num].move;

                if (run && show_currmove && ElapsedTimeMs() > 1000) {
                    UciHandler::SendToGui(GetCurrMoveInfo(curr_move, curr_move_num, settings.board));
                }

                settings.board.MakeMove(curr_move);
                normal_nodes++;
                Score score = -NegaMax(settings.board, -beta, -alpha, curr_max_depth - 1, 1);
                settings.board.UnmakeMove(curr_move);

                if (run) {
                    root_moves[curr_move_num].score = score;
                    if (multi_pv > 1) {
                        continue;
                    } else if (curr_move == best_move) {
                        best_score = score;
                    } else if (score > best_score) {
                        best_score = score;
                        best_move = curr_move;
                    }
                    if (score > alpha) {
                        alpha = score;
                    }
                }
            }

            SortRootNodes();
            if (run) {
                best_move = root_moves[0].move;
                best_score = root_moves[0].score;
                //tt.SaveEval(settings.board.GetZobristHash(), best_score, curr_max_depth, best_move, EXACT_SCORE, 0);
            }

            if (curr_max_depth > plies_muted) {
                UciHandler::SendToGui(GetSearchInfo(settings.board));
            }

            if (!run || !EnoughTimeLeft()) {
                break;
            }
        }

        run = false;
        search_timer.Stop();
        info_timer.Stop();
        UciHandler::SendToGui(GetBestMove());
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 0);
        }

        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
        if (score != NOT_FOUND) {
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
                return -MATE_SCORE + ply;
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

    bool ABSearch::EnoughTimeLeft() const {
        if (settings.infinite || settings.fixed_timer || settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    std::string ABSearch::GetBestMove() const {
        std::string ret = "bestmove ";
        ret.append(GetMoveName(best_move));
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

    void ABSearch::GenRootNodes() {
        MoveGen move_gen(settings.board);
        Move m;
        while ((m = move_gen.GetNextMove<false>())) {
            if (!settings.board.MakeMove(m)) {
                settings.board.UnmakeMove(m);
                continue;
            }
            Score s = tt.ProbeEval(settings.board.GetZobristHash(), NEGATIVE_INF, POSITIVE_INF, 0, 0);
            settings.board.UnmakeMove(m);
            if (s == NOT_FOUND) {
                s = MoveEval(settings.board, m);
            }
            root_moves[root_moves_cnt].move = m;
            root_moves[root_moves_cnt].score = s;
            root_moves_cnt++;
        }
    }

    void ABSearch::SortRootNodes() {
        std::stable_sort(std::execution::seq, root_moves, root_moves + root_moves_cnt,
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
        auto pvs_to_send = std::min(multi_pv, root_moves_cnt);
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info";
            if (pvs_to_send > 1) ss << " multipv " << i + 1;
            ss << " depth " << +curr_max_depth
               << " seldepth " << qsearch_depth + curr_max_depth
               << " nodes " << (normal_nodes + qsearch_nodes)
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score ";

            int mate_length_ply = 0;
            Score score = pvs_to_send > 1 ? root_moves[i].score : best_score;
            Move move = pvs_to_send > 1 ? root_moves[i].move : best_move;

            if (score >= MATE_SCORE - MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE - score);
                ss << "mate " << (mate_length_ply + 1) / 2;
            } else if (score <= -MATE_SCORE + MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE + score);
                ss << "mate " << -(mate_length_ply + 1) / 2;
            } else {
                ss << "cp " << score;
            }
            ss << " pv " << GetMoveName(move);
            board.MakeMove(move);
            RetrievePv(board, pv_stack, std::max(curr_max_depth - 1, mate_length_ply));
            board.UnmakeMove(move);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << " " << GetMoveName(pv_move);
            }
            ss << "\n";
        }

        return ss.str();
    }

    std::string ABSearch::GetCurrMoveInfo(Move move, int num, Board &board) {
        Move pv_stack[MAX_SEARCH_DEPTH];
        std::stringstream ss;
        ss << "info currmove " << GetMoveName(move) << " currmovenumber " << (num + 1);
        if (show_currline) {
            ss << " currline " << GetMoveName(move);
            board.MakeMove(move);
            RetrievePv(board, pv_stack, curr_max_depth - 1);
            board.UnmakeMove(move);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << " " << GetMoveName(pv_move);
            }
        }
        return ss.str();
    }
}
