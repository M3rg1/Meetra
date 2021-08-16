#include "SearchThread.h"
#include "Uci.h"
#include <sstream>
#include "MoveGen.h"

namespace Meetra {

    using namespace Search;

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        searching = true;

        // iterative deepening
        for (curr_depth = 2; curr_depth <= Globals::settings.max_allowed_depth && Globals::run; curr_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;

            // seldepth is always at least the current depth being searched
            if (IsMainThread()) {
                Globals::seldepth = curr_depth;
            }

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && curr_depth <= Globals::curr_max_depth) {
                curr_depth = Globals::curr_max_depth + thread_num;
            }

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size() && Globals::run; curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];

                if (IsMainThread() && Globals::show_currmove && ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo());
                }

                // if main thread already finished searching this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && curr_depth < Globals::curr_max_depth) {
                    break;
                }

                Globals::nodes_explored++;
                curr_rm->nodes++;
                board.MakeMove(curr_rm->move);
                Score score = -NegaMax(-beta, -alpha, curr_depth - 1, 2);
                board.UnmakeMove(curr_rm->move);

                if (Globals::run) {
                    if (score > alpha) {
                        alpha = score;
                        curr_rm->depth = curr_depth;
                        curr_rm->previous_score = curr_rm->score;
                        curr_rm->score = score;
                    } else {
                        curr_rm->previous_score = curr_rm->score;
                        curr_rm->score = NEGATIVE_INF;
                    }
                }
            }

            // sort based on score -> previous score -> node count
            std::stable_sort(root_moves.begin(), root_moves.end());

            // checking time and updating GUI when main thread finishes searching depth
            if (IsMainThread()) {

                Globals::curr_max_depth = curr_depth;

                // stop if we dont have enough time left for a deeper search or mate has been found within horizon and
                // we are not performing fixed time/depth/infinite or multipv search
                if (!EnoughTimeLeft() || (MateInHorizon() && Globals::multi_pv == 1 && !Globals::settings.infinite &&
                                          !Globals::settings.fixed_timer)) {
                    StopSearch();
                }

                // update GUI with info about currently finished depth we searched
                if (Globals::run && curr_depth > Globals::plies_muted) {
                    Uci::SendToGui(GetSearchInfo());
                }
            }
        }

        searching = false;
        StopSearch();
    }

    Score SearchThread::NegaMax(Score alpha, Score beta, Depth depth, Depth ply) {

        // terminating conditions, either we reached a draw - then stop, or max depth - in that case switch to qsearch
        if (board.IsRepetition() || board.Ply() >= Globals::plies_draw) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(alpha, beta, ply);
        }

        // this position has already been probed for this or deeper depth and we have the results in TT
        Score score = Globals::tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
        if (score != TT_NOT_FOUND) {
            return score;
        }

        MoveGen move_gen(board, &Globals::tt);
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;
        Move move;

        // iterate over available moves
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            Globals::nodes_explored++;
            curr_rm->nodes++;
            score = -NegaMax(-beta, -alpha, depth - 1, ply + 1);
            board.UnmakeMove(move);

            if (!Globals::run) {
                return 0;
            } else if (score > alpha) {
                if (score >= beta) {
                    Globals::tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
                    return beta;
                }
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move_this_iter = move;
            }
        }

        // if score == TT_NOT_FOUND, that means we didnt search a single move, that means there are no legal moves
        // in this position, that means we are either in check-mate or a stalemate
        if (score == TT_NOT_FOUND) {
            // king in check = checkmate
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply;
            }
            // else stalemate
            return -DRAW_SCORE;
        }

        // we have improved alpha, meaning this position is our PV, store it in PVTable
        if (tt_flag == EXACT_SCORE) {
            Globals::pvt.SavePv(board.GetZobristHash(), best_move_this_iter);
        }

        // whatever we learnt about this position, store it in TT for later use
        Globals::tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

        return alpha;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        // update seldepth for this root move
        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        if (IsMainThread()) {
            Globals::seldepth = std::max(ply, Globals::seldepth);
        }

        // stand pat
        auto score = Evaluation::BoardEval(board);
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        MoveGen move_gen(board, &Globals::tt);
        Move move;
        // iterate over all available captures
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            Globals::nodes_explored++;
            score = -QSearch(-beta, -alpha, ply + 1);
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

    bool SearchThread::MateInHorizon() const {
        if(root_moves[0].score == NEGATIVE_INF){
            return false;
        }
        if (std::abs(root_moves[0].score) > MIN_MATE_EVAL) {
            int distance_to_mate = MATE_SCORE - std::abs(root_moves[0].score);
            if (curr_depth > distance_to_mate) {
                return true;
            }
        }
        return false;
    }

    void SearchThread::RetrievePv(Move *pv_line, Depth depth) {
        Move move = Globals::pvt.ProbePv(board.GetZobristHash());
        // TODO if pv not found in pvt, try searching TT, if not in TT go back to pvt, if neither, too bad
        if (!move || depth == 0 || board.IsRepetition() || board.Ply() >= Globals::plies_draw) {
            *pv_line = INVALID_MOVE;
            return;
        }
        *pv_line++ = move;
        board.MakeMove(move);
        RetrievePv(pv_line, depth - 1);
        board.UnmakeMove(move);
    }

    std::string SearchThread::GetSearchInfo() {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(Globals::nodes_explored) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        auto pvs_to_send = std::min(static_cast<size_t>(Globals::multi_pv), root_moves.size());
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info";
            if (pvs_to_send > 1) ss << " multipv " << i + 1;
            ss << " depth " << static_cast<int>(root_moves[i].depth)
               << " seldepth "
               << std::max(static_cast<int>(root_moves[i].seldepth), static_cast<int>(root_moves[i].depth))
               << " nodes " << Globals::nodes_explored
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(Globals::tt.Usage() * 1000)
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
            RetrievePv(pv_stack, std::max(32, distance_to_mate));
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

    std::string SearchThread::GetCurrMoveInfo() {
        Move pv_stack[MAX_SEARCH_DEPTH];
        std::stringstream ss;
        ss << "info currmove " << GetMoveName(curr_rm->move) << " currmovenumber " << (curr_rm_num + 1);
        if (Globals::show_currline) {
            ss << " currline " << GetMoveName(curr_rm->move);
            board.MakeMove(curr_rm->move);
            RetrievePv(pv_stack, curr_depth);
            board.UnmakeMove(curr_rm->move);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << " " << GetMoveName(pv_move);
            }
        }
        return ss.str();
    }


}