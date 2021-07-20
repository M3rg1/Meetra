#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "Uci.h"
#include "iostream"
#include <random>
#include <algorithm>

namespace Meetra {


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
        root_moves_cnt = 0;
        curr_max_depth = 0;

        GenRootMoves(board);

        settings.max_allowed_depth = std::min(settings.max_allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);

        if (!settings.infinite) {
            search_timer.SetTimeout([&]() { StopSearch(); }, settings.allowed_time);
        }

        info_timer.SetInterval([&]() {
            Uci::SendToGui(GetUpdateSearchInfo());
        }, settings.info_to_ui_ms_timer);
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

    void ABSearch::StartSearch(SearchSettings s, Board board) {

        InitSearch(s, board);

        if (root_moves_cnt < 2) {
            StopSearch();
            Move only_move = root_moves_cnt == 0 ? INVALID_MOVE : root_moves[0].move;
            Uci::SendToGui("bestmove " + GetMoveName(only_move));
            return;
        }

        for (int thread_num = 0; thread_num < num_threads; thread_num++) {

            futures.push_back(ThreadPool::PushTask([=, this]() mutable {

                MoveAndNodes root[MAX_LEGAL_MOVES];
                std::random_device rd;
                std::mt19937 g(rd());
                long best_idx;
                int curr_depth;
                Depth max_ply;

                for (curr_depth = 2; curr_depth <= settings.max_allowed_depth; curr_depth++) {

                    Score alpha = NEGATIVE_INF;
                    Score beta = POSITIVE_INF;
                    max_ply = 0;
                    best_idx = 0;

                    {
                        std::scoped_lock lock(mtx);
                        memcpy(root, root_moves, root_moves_cnt * sizeof(MoveAndNodes));
                        if (curr_max_depth >= curr_depth) {
                            curr_depth = curr_max_depth + 1;
                        }
                    }
                    std::shuffle(root + 1, root + root_moves_cnt, g);

                    for (int curr_move_num = 0; curr_move_num < root_moves_cnt; curr_move_num++) {

                        Move curr_move = root[curr_move_num].move;
                        if (show_currmove && ElapsedTimeMs() > 1000 && thread_num == 0) {
                            Uci::SendToGui(GetCurrMoveInfo(curr_move, curr_move_num, board));
                        }

                        ulong nodes = 1;
                        board.MakeMove(curr_move);
                        nodes_explored++;
                        Score score = -NegaMax(board, -beta, -alpha, curr_depth - 1, 2, max_ply, nodes);
                        board.UnmakeMove(curr_move);

                        if (run) {
                            if (curr_depth <= curr_max_depth) {
                                break;
                            }
                            root[curr_move_num].seldepth = max_ply;
                            root[curr_move_num].nodes = nodes;
                            root[curr_move_num].score = score;
                            root[curr_move_num].depth = curr_depth;
                            if (score > alpha && multi_pv == 1) {
                                alpha = score;
                                if (score > root[best_idx].score) {
                                    best_idx = curr_move_num;
                                }
                            }
                        }
                    }

                    {
                        std::scoped_lock lock(mtx);
                        if (run && curr_depth > curr_max_depth) {
                            curr_max_depth = curr_depth;
                            std::swap(root[0], root[best_idx]);
                            std::sort(root + 1, root + root_moves_cnt, CompNodesLesserMAN);
                            memcpy(root_moves, root, sizeof(MoveAndNodes) * root_moves_cnt);

                            if (curr_depth > plies_muted) {
                                Uci::SendToGui(GetSearchInfo(board));
                            }

                            if (std::abs(root[0].score) > MIN_MATE_EVAL && multi_pv == 1 && !settings.infinite &&
                                !settings.fixed_timer) {
                                int distance_to_mate = MATE_SCORE - std::abs(root[0].score);
                                if (curr_depth > distance_to_mate) {
                                    StopSearch();
                                }
                            }
                        }
                    }

                    if (!run || !EnoughTimeLeft()) {
                        break;
                    }
                }
                {
                    std::scoped_lock lock(mtx);
                    // find the best move that wasn't refuted - root_moves[0] might not be root[0] anymore, since other
                    // thread might've already replaced it, so we must find the corresponding move in our root move list
                    // and check whether we've refuted that move, if we did, we have new best root_moves[0] move
                    for (int i = 0; i < root_moves_cnt; i++) {
                        if (root_moves[0].move == root[i].move &&
                            root_moves[0].score <= root[i].score &&
                            root_moves[0].depth <= root[i].depth) {
                            root_moves[0] = root[best_idx];
                        }
                    }
                }
            }));
        }
        for (auto &future : futures) {
            future.wait();
        }
        futures.clear();
        Uci::SendToGui(GetSearchInfo(board));
        Uci::SendToGui("bestmove " + GetMoveName(root_moves[0].move));
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply, Depth &max_reached_ply,
                            ulong &nodes) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, ply, max_reached_ply, nodes);
        }

        if (ply > max_reached_ply) {
            max_reached_ply = ply;
        }

        Move move;
        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply, move);
        if (score != NOT_FOUND) {
            if (move != INVALID_MOVE) {
                BackupPv(board, depth);
            }
            return score;
        }

        MoveGen move_gen(board, &tt);
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;

        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_explored++;
            nodes++;
            score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, max_reached_ply, nodes);
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

    Score
    ABSearch::QuiescenceSearch(Board &board, Score alpha, Score beta, Depth ply, Depth &max_reached_ply, ulong &nodes) {

        if (ply > max_reached_ply) {
            max_reached_ply = ply;
        }

        auto score = Evaluation::BoardEval(board);
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        // TODO in qsearch try not to order moves by position, just by victim/attacker .. maybe?
        MoveGen move_gen(board, &tt);
        Move move;
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_explored++;
            nodes++;
            score = -QuiescenceSearch(board, -beta, -alpha, ply + 1, max_reached_ply, nodes);
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
        if (!move || depth == 0 || board.IsRepetition() || board.Ply() >= 50) {
            *pv_line = INVALID_MOVE;
            return;
        }
        *pv_line++ = move;
        board.MakeMove(move);
        RetrievePv(board, pv_line, depth - 1);
        board.UnmakeMove(move);
    }

    void ABSearch::GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            board.UnmakeMove(move);
            root_moves[root_moves_cnt].move = move;
            root_moves[root_moves_cnt].depth = 0;
            root_moves_cnt++;
        }
    }

    std::string ABSearch::GetUpdateSearchInfo() const {

        auto elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_explored) * 1000.0 / static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << static_cast<int>(root_moves[0].depth)
           << " seldepth " << static_cast<int>(root_moves[0].seldepth)
           << " nodes " << (nodes_explored)
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull " << static_cast<int>(tt.Usage() * 1000);

        return ss.str();
    }

    std::string ABSearch::GetSearchInfo(Board &board) {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_explored) * 1000.0 / static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        auto pvs_to_send = std::min(multi_pv, root_moves_cnt);
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info";
            if (pvs_to_send > 1) ss << " multipv " << i + 1;
            ss << " depth " << static_cast<int>(root_moves[i].depth)
               << " seldepth " << static_cast<int>(root_moves[i].seldepth)
               << " nodes " << nodes_explored
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score ";

            Score score = root_moves[i].score;
            Move move = root_moves[i].move;
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
