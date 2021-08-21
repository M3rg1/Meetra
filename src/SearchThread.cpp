#include "SearchThread.h"
#include "Uci.h"
#include <sstream>
#include "MoveGen.h"
#include <random>
#include <ctime>
#include <cstdlib>
#include "Search.h"

namespace Meetra {


    Search::RootMove SearchThread::GetBestRootMove() const { return root_moves[0]; };

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (curr_depth = 2; curr_depth <= Search::Globals::settings.max_allowed_depth && Search::Run(); curr_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;

            // seldepth is always at least the current depth being searched
            if (IsMainThread()) {
                Search::Globals::seldepth.store(curr_depth, std::memory_order_relaxed);
            }

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && curr_depth <= Search::Globals::curr_max_depth.load(std::memory_order_relaxed)) {
                curr_depth = Search::Globals::curr_max_depth.load(std::memory_order_relaxed) + thread_num;
            }

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = curr_depth;

                if (IsMainThread() && Search::Globals::show_currmove && Search::ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo());
                }

                // if main thread already finished active this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && curr_depth < Search::Globals::curr_max_depth.load(std::memory_order_relaxed)) {
                    break;
                }

                Search::Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
                curr_rm->nodes++;
                board.MakeMove(curr_rm->move);
                Score score = -NegaMax(-beta, -alpha, curr_depth - 1, 2, curr_rm->pv);
                board.UnmakeMove(curr_rm->move);

                if (Search::Run()) {
                    curr_rm->previous_score = curr_rm->score;
                    curr_rm->depth = curr_depth;
                    if (Search::Globals::multi_pv > 1) {
                        curr_rm->score = score;
                    } else if (score > alpha) {
                        curr_rm->score = score;
                        alpha = score;
                    } else {
                        curr_rm->score = NEGATIVE_INF;
                    }
                } else {
                    break;
                }
            } // end alpha beta loop

            // sort based on score -> previous score -> node count
            std::stable_sort(root_moves.begin(), root_moves.end());

            // checking time and updating GUI when main thread finishes active depth
            if (IsMainThread()) {

                Search::Globals::curr_max_depth.store(curr_depth, std::memory_order_relaxed);

                // active if we don't have enough time left for a deeper search or mate has been found, and we are not
                // performing fixed time/depth/infinite or multipv search
                if (!Search::EnoughTimeLeft() || Search::TimeRunOut() ||
                (MateInHorizon() && Search::Globals::multi_pv == 1 && !Search::Globals::settings.infinite &&
                    !Search::Globals::settings.fixed_timer)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (Search::Run() && curr_depth > Search::Globals::plies_muted) {
                    Uci::SendToGui(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        Search::StopSearch();
        active = false;
        if (IsMainThread()) {
            Search::FinishSearch();
        }
    }

    Score SearchThread::NegaMax(Score alpha, Score beta, Depth depth, Depth ply, std::vector<Move> &pv_line) {

        if(IsMainThread() && (curr_rm->nodes & 8191) == 0 && Search::TimeRunOut()) {
            Search::StopSearch();
            return 0;
        }

        // terminating conditions, either we reached a draw, or max depth - in that case switch to qsearch
        if (board.IsRepetition() || board.Ply() >= Search::Globals::plies_draw) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(alpha, beta, ply);
        }

        EntryFlag tt_flag;
        Score score;
        Move best_move;
        MoveGen move_gen(board);
        Search::Globals::tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply, score, tt_flag, best_move);

        // do a check of the retrieved move, if it's legal to play in the current position and not corrupted,
        // chances are, the score is correct as well
        if (move_gen.IsPseudoLegal(best_move)) {
            // we have a good match and will be making a cutoff
            if (tt_flag == ALPHA || tt_flag == BETA) {
                return score;
            }
            // no cutoff, but we got some move from TT, we will play it as the first move in the main negamax loop
            if (best_move != ZERO_MOVE) {
                move_gen.PutTTMove(best_move);
            }
        }


        Move move;
        tt_flag = ALPHA;
        bool moves_available = false;

        while ((move = move_gen.GetBestMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            moves_available = true;
            std::vector<Move> line;
            Search::Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
            score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, line);
            board.UnmakeMove(move);

            if (!Search::Run()) {
                return 0;
            } else if (score > alpha) {
                if (score >= beta) {
                    Search::Globals::tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
                    return beta;
                }
                pv_line.clear();
                pv_line.emplace_back(move);
                pv_line.insert(pv_line.begin() + 1, line.begin(), line.end());
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move = move;
            }
        }

        if (!moves_available) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        }

        // whatever we learnt about this position, store it in TT for later use
        Search::Globals::tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move, tt_flag, ply);

        return alpha;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        // update seldepth for this root move
        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        if (IsMainThread()) {
            Search::Globals::seldepth.store(std::max(ply, Search::Globals::seldepth.load(std::memory_order_relaxed)),
                                    std::memory_order_relaxed);
        }

        // stand pat
        auto score = Evaluation::BoardEval(board);
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        MoveGen move_gen(board);
        Move move;
        // iterate over all available captures
        while ((move = move_gen.GetBestMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            Search::Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
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
        if (root_moves[0].score != NEGATIVE_INF && std::abs(root_moves[0].score) > MIN_MATE_EVAL) {
            int distance_to_mate = MATE_SCORE - std::abs(root_moves[0].score);
            if (curr_depth > distance_to_mate) {
                return true;
            }
        }
        return false;
    }

    bool SearchThread::MateFound() const {
        return root_moves[0].score != NEGATIVE_INF && std::abs(root_moves[0].score) > MIN_MATE_EVAL;
    }

    std::string SearchThread::GetSearchInfo() {

        std::ostringstream oss;
        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(Search::Globals::nodes_explored.load(std::memory_order_relaxed)) /
                  static_cast<double>(elapsed_ms))) * 1000.0);
        auto pvs_to_send = std::min(static_cast<size_t>(Search::Globals::multi_pv), root_moves.size());

        for (auto i = 0; i < pvs_to_send; i++) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << i + 1;
            oss << " depth " << static_cast<int>(root_moves[i].depth)
                << " seldepth " << static_cast<int>(root_moves[i].seldepth)
                << " nodes " << Search::Globals::nodes_explored.load(std::memory_order_relaxed)
                << " time " << elapsed_ms
                << " nps " << nps
                << " hashfull " << static_cast<int>(Search::Globals::tt.Usage() * 1000)
                << " score ";

            Score score = root_moves[i].score;
            Move move = root_moves[i].move;
            if (score > MIN_MATE_EVAL) {
                int distance_to_mate = static_cast<int>(MATE_SCORE - score);
                oss << "mate " << (distance_to_mate) / 2;
            } else if (score < -MIN_MATE_EVAL) {
                int distance_to_mate = static_cast<int>(MATE_SCORE + score);
                oss << "mate " << -(distance_to_mate) / 2;
            } else {
                oss << "cp " << score;
            }

            oss << " pv " << GetMoveName(move);
            for (Move pv_move : root_moves[i].pv) {
                oss << ' ' << GetMoveName(pv_move);
            }
            if (i + 1 < pvs_to_send) {
                oss << '\n';
            }
        }

        return oss.str();
    }

    std::string SearchThread::GetCurrMoveInfo() {
        return "info currmove " + GetMoveName(curr_rm->move) + " currmovenumber " + std::to_string(curr_rm_num + 1);
    }

    std::string SearchThread::GetCurrLineInfo() {
        std::string ret = "info currline ";
        ret += GetMoveName(curr_rm->move);
        for (Move pv_move : curr_rm->pv) {
            ret += ' ' + GetMoveName(pv_move);
        }
        return ret;
    }
}