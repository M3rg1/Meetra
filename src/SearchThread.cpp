#include "SearchThread.h"
#include "Uci.h"
#include <sstream>
#include "MoveGen.h"

namespace Meetra {

    using namespace Search;

    void SearchThread::SearchThread::Search() {

        for (curr_depth = 2; curr_depth <= Globals::settings.max_allowed_depth && Globals::run; curr_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;

            if (IsMainThread()) {
                Globals::seldepth = curr_depth;
            }

            if (!IsMainThread() && curr_depth <= Globals::curr_max_depth) {
                curr_depth = Globals::curr_max_depth + thread_num;
            }

            for (curr_rm_num = 0; curr_rm_num < root_moves.size() && Globals::run; curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];

                if (IsMainThread() && Globals::show_currmove && ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo());
                }

                if (curr_depth < Globals::curr_max_depth) {
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
                        curr_rm->score = score;
                        if (score > root_moves[best_rm_num].score) {
                            best_rm_num = curr_rm_num;
                        }
                    } else {
                        curr_rm->score = NEGATIVE_INF;
                    }
                }
            }

            std::swap(root_moves[0], root_moves[best_rm_num]);
            best_rm_num = 0;
            std::sort(root_moves.begin() + 1, root_moves.end());

            if (IsMainThread()) {

                Globals::curr_max_depth = curr_depth;

                if (!EnoughTimeLeft() || MateInHorizon()) {
                    StopSearch();
                }

                if (curr_depth > Globals::plies_muted) {
                    Uci::SendToGui(GetSearchInfo());
                }
            }
        }

        // TODO remove this and just get the best move from all the threads back in the Search method
        if (IsMainThread()) {
            Globals::main_move = root_moves[0].move;
        }
    }

    Score SearchThread::SearchThread::NegaMax(Score alpha, Score beta, Depth depth, Depth ply) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(alpha, beta, ply);
        }

        Score score = Globals::tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
        if (score != TT_NOT_FOUND) {
            return score;
        }

        MoveGen move_gen(board, &Globals::tt);
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;
        Move move;

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


        if (score == TT_NOT_FOUND) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        }

        if (tt_flag == EXACT_SCORE) {
            Globals::pvt.SavePv(board.GetZobristHash(), best_move_this_iter);
        }

        Globals::tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

        return alpha;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        if (IsMainThread()) {
            Globals::seldepth = std::max(ply, Globals::seldepth);
        }

        auto score = Evaluation::BoardEval(board);
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        MoveGen move_gen(board, &Globals::tt);
        Move move;
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
        if (std::abs(root_moves[0].score) > MIN_MATE_EVAL && Globals::multi_pv == 1 && !Globals::settings.infinite &&
            !Globals::settings.fixed_timer) {
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
        if (!move || depth == 0 || board.IsRepetition() || board.Ply() >= 50) {
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
            ss << " depth " << static_cast<int>(curr_depth)
               << " seldepth " << static_cast<int>(root_moves[i].seldepth)
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