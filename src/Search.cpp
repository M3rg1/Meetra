#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "Uci.h"
#include <iostream>
#include <random>

namespace Meetra {

#define MAIN_THREAD 0
#define IS_MAIN_THREAD(x) x == MAIN_THREAD
#define IS_HELPER_THREAD(x) x != MAIN_THREAD


    ABSearch::ABSearch() {
        run = false;
        show_currline = false;
        multi_pv = 1;
        show_currmove = true;
        plies_muted = 1;
        num_threads = DEFAULT_SEARCH_THREADS;
    }

    void ABSearch::InitSearch(SearchSettings &s, Board &board) {

        run = true;

        timer_start = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();

        settings = s;

        tt.NewSearch();
        pvt.NewSearch();
        nodes_explored = 0;
        curr_max_depth = 0;
        qsearch_depth = 0;
        main_move = INVALID_MOVE;

        settings.max_allowed_depth = std::min(settings.max_allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);

        if (!settings.infinite) {
            search_timer.SetTimeout([&]() { StopSearch(); }, settings.allowed_time);
        }

        info_timer.SetInterval([&]() {
            Uci::SendToGui(GetUpdateSearchInfo());
        }, settings.info_to_ui_ms_timer);
    }

    void ABSearch::StartSearch(SearchSettings s, Board board) {

        InitSearch(s, board);
        auto root_moves = GenRootMoves(board);

        if (root_moves.size() < 2 && !settings.infinite) {
            StopSearch();
            Move only_move = root_moves.empty() ? INVALID_MOVE : root_moves[0].move;
            Uci::SendToGui("bestmove " + GetMoveName(only_move));
            return;
        }

/*        for (auto &m : root_moves) {
            board.MakeMove(m.move);
            m.score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, 3, 2, 1);
            board.UnmakeMove(m.move);
        }*/

        std::sort(root_moves.begin(), root_moves.end());

        for (int t = num_threads - 1; t >= MAIN_THREAD; t--) {
            futures.push_back(ThreadPool::PushTask([=, this]() mutable {
                MainSearch(board, t, root_moves);
            }));
        }

        futures.back().wait();
        futures.pop_back();

        Uci::SendToGui("bestmove " + GetMoveName(main_move));

        for (auto &future : futures) {
            future.wait();
        }
        futures.clear();
    }

    void ABSearch::MainSearch(Board board, int thread, std::vector<p_MoveNodes> moves) {

        for (int curr_depth = 2; curr_depth <= settings.max_allowed_depth && run; curr_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            int best_idx = 0;

            if (IS_MAIN_THREAD(thread)) {
                curr_max_depth = curr_depth;
            } else if (curr_depth <= curr_max_depth) {
                curr_depth = curr_max_depth + thread;
            }

            for (int curr_move_num = 0; curr_move_num < moves.size() && run; curr_move_num++) {
                Move curr_move = moves[curr_move_num].move;

                if (IS_MAIN_THREAD(thread) && show_currmove && ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo(curr_move, curr_move_num, board));
                }

                if (curr_depth < curr_max_depth) {
                    break;
                }

                nodes_explored++;
                uint nodes = 1;
                board.MakeMove(curr_move);
                Score score = -NegaMax(board, -beta, -alpha, curr_depth - 1, 2, thread, nodes);
                board.UnmakeMove(curr_move);

                if (run) {
                    moves[curr_move_num].nodes = nodes;
                    if (score > alpha) {
                        alpha = score;
                        moves[curr_move_num].score = score;
                        if (score > moves[best_idx].score) {
                            best_idx = curr_move_num;
                        }
                    } else {
                        moves[curr_move_num].score = NEGATIVE_INF;
                    }
                }
            }

            std::swap(moves[0], moves[best_idx]);
            std::sort(moves.begin() + 1, moves.end());

            if (IS_MAIN_THREAD(thread)) {

                if (curr_depth > curr_max_depth) {
                    curr_max_depth = curr_depth;
                }

                if (!EnoughTimeLeft() || MateInHorizon(curr_depth, moves[0].score)) {
                    StopSearch();
                }

                if (curr_depth > plies_muted) {
                    Uci::SendToGui(GetSearchInfo(board, moves));
                }
            }
        }

        if (IS_MAIN_THREAD(thread)) {
            main_move = moves[0].move;
        }
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply, int thread, uint &nodes) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(board, alpha, beta, 0, thread, nodes);
        }

        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
        if (score != NOT_FOUND) {
            return score;
        }

        MoveGen move_gen(board, &tt);
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;
        Move move;

        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_explored++;
            nodes++;
            score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, thread, nodes);
            board.UnmakeMove(move);

            if (!run) {
                return 0;
            } else if (score > alpha) {
                if (score >= beta) {
                    tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
                    return beta;
                }
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move_this_iter = move;
            }
        }


        if (score == NOT_FOUND) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        }

        if (tt_flag == EXACT_SCORE) {
            pvt.SavePv(board.GetZobristHash(), best_move_this_iter);
        }

        tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

        return alpha;
    }

    Score ABSearch::QSearch(Board &board, Score alpha, Score beta, Depth ply, int thread, uint &nodes) {

        if (IS_MAIN_THREAD(thread) && ply > qsearch_depth) {
            qsearch_depth = ply;
        }

        auto score = Evaluation::BoardEval(board);
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        MoveGen move_gen(board, &tt);
        Move move;
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes++;
            nodes_explored++;
            score = -QSearch(board, -beta, -alpha, ply + 1, thread, nodes);
            board.UnmakeMove(move);
            if (score > alpha) {
                if (score >= beta) {
                    return beta;
                }
                alpha = score;
            }
        }

        return alpha;
    }

    bool ABSearch::MateInHorizon(Depth curr_depth, Score score) const {
        if (std::abs(score) > MIN_MATE_EVAL && multi_pv == 1 && !settings.infinite &&
            !settings.fixed_timer) {
            int distance_to_mate = MATE_SCORE - std::abs(score);
            if (curr_depth > distance_to_mate) {
                return true;
            }
        }
        return false;
    }

    bool ABSearch::EnoughTimeLeft() const {
        if (settings.infinite || settings.fixed_timer || settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    long ABSearch::ElapsedTimeMs() const {
        long now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        return std::max(1l, elapsed_ms);
    }

    void ABSearch::BackupPv(Board &board, Depth depth) {
        Move move = tt.GetPVMove(board.GetZobristHash());
        if (!move || depth == 0) {
            return;
        }
        pvt.SavePv(board.GetZobristHash(), move);
        board.MakeMove(move);
        BackupPv(board, depth - 1);
        board.UnmakeMove(move);
    }

    void ABSearch::RetrievePv(Board &board, Move *pv_line, Depth depth) const {
        Move move = pvt.ProbePv(board.GetZobristHash());
        // TODO if pv not found in pvt, try searching TT, if not in TT go back to pvt, if neither, too bad
        if (!move || depth == 0 || board.IsRepetition() || board.Ply() >= 50) {
            *pv_line = INVALID_MOVE;
            return;
        }
        *pv_line++ = move;
        board.MakeMove(move);
        RetrievePv(board, pv_line, depth - 1);
        board.UnmakeMove(move);
    }

    std::vector<ABSearch::p_MoveNodes> ABSearch::GenRootMoves(Board &board) const {
        MoveGen move_gen(board);
        Move move;
        std::vector<p_MoveNodes> moves;
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            board.UnmakeMove(move);
            moves.emplace_back(move);
        }
        return moves;
    }

    void ABSearch::InitSearchTimer(Board &board) {

        if (settings.infinite || settings.fixed_timer) {
            return;
        }

        auto time_left = board.ColorToMove() == WHITE ? settings.white_time : settings.black_time;
        if (time_left) {
            int moves_made = std::min(static_cast<int>(board.MovesMadeCount() + 1), 10);
            double factor = 2.0 - moves_made / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - moves_made;
            settings.allowed_time = static_cast<long>(factor * target);
        }
    }

    std::string ABSearch::GetUpdateSearchInfo() const {

        auto elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_explored) * 1000.0 / static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << static_cast<int>(curr_max_depth)
           << " seldepth " << static_cast<int>(qsearch_depth + curr_max_depth)
           << " nodes " << (nodes_explored)
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull " << static_cast<int>(tt.Usage() * 1000);

        return ss.str();
    }

    std::string ABSearch::GetSearchInfo(Board &board, std::vector<p_MoveNodes> &moves) {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_explored) * 1000.0 / static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        auto pvs_to_send = std::min(multi_pv, moves.size());
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info";
            if (pvs_to_send > 1) ss << " multipv " << i + 1;
            ss << " depth " << static_cast<int>(curr_max_depth)
               << " seldepth " << static_cast<int>(qsearch_depth + curr_max_depth)
               << " nodes " << nodes_explored
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score ";

            Score score = moves[i].score;
            Move move = moves[i].move;
            int distance_to_mate = 0;
            if (score > MIN_MATE_EVAL) {
                distance_to_mate = static_cast<int>(MATE_SCORE - score);
                ss << "mate " << (distance_to_mate) / 2;
            } else if (score < -MIN_MATE_EVAL) {
                distance_to_mate = static_cast<int>(MATE_SCORE + score);
                ss << "mate " << -(distance_to_mate) / 2;
            } else {
                ss << "cp " << score;
            }
            ss << " pv " << GetMoveName(move);
            board.MakeMove(move);
            RetrievePv(board, pv_stack, std::max(32, distance_to_mate));
            board.UnmakeMove(move);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << ' ' << GetMoveName(pv_move);
            }
            ss << '\n';
        }

        return ss.str();
    }

    std::string ABSearch::GetCurrMoveInfo(Move move, int num, Board &board) const {
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
