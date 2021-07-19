#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "Uci.h"
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

    void ABSearch::InitSearch(SearchSettings &s, Board &board) {

        timer_start = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();

        settings = s;

        tt.NewSearch();
        nodes_explored = 0;
        qsearch_depth = 0;
        curr_max_depth = 0;
        root_moves_cnt = 0;

        GenRootMoves(board);

        best_score = NEGATIVE_INF;
        best_move = root_moves[0].move;

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
        run = true;
        InitSearch(s, board);

        if (root_moves_cnt < 2) {
            StopSearch();
            Move bm = root_moves_cnt == 0 ? INVALID_MOVE : root_moves[0].move;
            Uci::SendToGui("bestmove " + GetMoveName(bm));
            return;
        }

/*        for(int i = 0; i < root_moves_cnt; i++){
            root_moves[i].score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, 5, 1);
        }*/

        int best_idx = 0;
        Score best_score = NEGATIVE_INF;
        for (curr_max_depth = 2; curr_max_depth <= settings.max_allowed_depth; curr_max_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            int best_idx_this_iter = 0;
            qsearch_depth = 0;

            for (int curr_move_num = 0; curr_move_num < root_moves_cnt; curr_move_num++) {

                Move curr_move = root_moves[curr_move_num].move;

                if (show_currmove && ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo(curr_move, curr_move_num, board));
                }

                ulong nodes = 0;
                board.MakeMove(curr_move);
                Score score = -NegaMax(board, -beta, -alpha, curr_max_depth - 1, 2, nodes);
                board.UnmakeMove(curr_move);
                nodes_explored += nodes;

                if (run) {
                    root_moves[curr_move_num].nodes = nodes;
                    root_moves[curr_move_num].score = score;
                    if (score > alpha && multi_pv == 1) {
                        alpha = score;
                        best_idx_this_iter = curr_move_num;
                        if(score > best_score){
                            best_score = score;
                            best_idx = curr_move_num;
                        }
                    }
                }
            }

            if(multi_pv == 1){
                if (run) {
                    best_idx = best_idx_this_iter;
                }
                std::swap(root_moves[0], root_moves[best_idx]);
                best_score = root_moves[0].score;
                std::sort(root_moves + 1, root_moves + root_moves_cnt, CompMaNNodes);
            } else {
                std::sort(root_moves, root_moves + root_moves_cnt, CompMaNScore);
            }

            if (!settings.infinite && !settings.fixed_timer && std::abs(best_score) > MIN_MATE_EVAL) {
                int distance_to_mate = MATE_SCORE - std::abs(best_score);
                // TODO if mate is beyond horizon instead of showing in gui MATE in X, show some score
                // otherwise the mate is going up and down randomly its shiiet
                if (curr_max_depth > distance_to_mate && multi_pv == 1) {
                    run = false;
                }
            }

            if (curr_max_depth > plies_muted) {
                Uci::SendToGui(GetSearchInfo(board));
            }

            if (!run || !EnoughTimeLeft()) {
                break;
            }
        }

        StopSearch();
        Uci::SendToGui(GetBestMove());
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply, ulong &nodes) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 0, nodes);
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
            nodes++;
            score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, nodes);
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
            return -DRAW_SCORE;
        }

        tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

        return alpha;
    }

    Score ABSearch::QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth, ulong &nodes) {

        if (depth > qsearch_depth) {
            qsearch_depth = depth;
        }

        auto score = Evaluation::BoardEval(board);
        if (score >= beta) {
            return beta;
        } else if (score > alpha) {
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
            nodes++;
            score = -QuiescenceSearch(board, -beta, -alpha, depth + 1, nodes);
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
        return "bestmove " + GetMoveName(root_moves[0].move);
    }

    long ABSearch::ElapsedTimeMs() const {
        long now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        return std::max(1l, elapsed_ms);
    }

    void ABSearch::RetrievePv(Board &board, Move *pv_line, Depth depth) const {
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

    void ABSearch::GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            //Score score = tt.ProbeEval(board.GetZobristHash(), NEGATIVE_INF, POSITIVE_INF, 0, 1);
            board.UnmakeMove(move);
            //if (score == NOT_FOUND) score = Evaluation::MoveEval(board, move);
            root_moves[root_moves_cnt].move = move;
            //root_moves[root_moves_cnt].score = score;
            root_moves_cnt++;
        }
    }

    std::string ABSearch::GetUpdateSearchInfo() const {

        auto elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_explored) * 1000.0 / static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << static_cast<int>(curr_max_depth)
           << " seldepth " << qsearch_depth + curr_max_depth
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
            ss << " depth " << static_cast<int>(curr_max_depth)
               << " seldepth " << qsearch_depth + curr_max_depth
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
            RetrievePv(board, pv_stack, std::max(curr_max_depth - 1, distance_to_mate));
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
