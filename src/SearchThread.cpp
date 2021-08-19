#include "SearchThread.h"
#include "Uci.h"
#include <sstream>
#include "MoveGen.h"

namespace Meetra {

    using namespace Search;

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (curr_depth = 2; curr_depth <= Globals::settings.max_allowed_depth && Run(); curr_depth++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;

            // seldepth is always at least the current depth being searched
            if (IsMainThread()) {
                Globals::seldepth.store(curr_depth, std::memory_order_relaxed);
            }

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && curr_depth <= Globals::curr_max_depth.load(std::memory_order_relaxed)) {
                curr_depth = Globals::curr_max_depth.load(std::memory_order_relaxed) + thread_num;
            }

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = curr_depth;

                if (IsMainThread() && Globals::show_currmove && ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo());
                }

                // if main thread already finished active this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && curr_depth < Globals::curr_max_depth.load(std::memory_order_relaxed)) {
                    break;
                }

                Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
                curr_rm->nodes++;
                board.MakeMove(curr_rm->move);
                Score score = -NegaMax(-beta, -alpha, curr_depth - 1, 2, curr_rm->pv);
                board.UnmakeMove(curr_rm->move);

                if (Run()) {
                    curr_rm->previous_score = curr_rm->score;
                    curr_rm->depth = curr_depth;
                    if (Globals::multi_pv > 1) {
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

                Globals::curr_max_depth.store(curr_depth, std::memory_order_relaxed);

                // active if we don't have enough time left for a deeper search or mate has been found, and we are not
                // performing fixed time/depth/infinite or multipv search
                if (!EnoughTimeLeft() ||
                    (MateInHorizon() && Globals::multi_pv == 1 && !Globals::settings.infinite &&
                     !Globals::settings.fixed_timer)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (Run() && curr_depth > Globals::plies_muted) {
                    Uci::SendToGui(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        StopSearch();
        active = false;
        if(IsMainThread()){
            FinishSearch();
        }
    }

    Score SearchThread::NegaMax(Score alpha, Score beta, Depth depth, Depth ply, std::vector<Move> &pv_line) {

        // terminating conditions, either we reached a draw - then active, or max depth - in that case switch to qsearch
        if (board.IsRepetition() || board.Ply() >= Globals::plies_draw) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(alpha, beta, ply);
        }

        // this position has already been probed for this or deeper depth, and we have the results in TT
        EntryFlag tt_flag;
        Score score;
        Globals::tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply, score, tt_flag);
        // we continue active if exact TT hit to collect the pv line (little overhead, could just return score)
        if (tt_flag != NOT_FOUND) {
            if(tt_flag == EXACT_SCORE) {
                pv_line.clear();
                BackupPv(pv_line, board, depth);
            }
            return score;
        }

        MoveGen move_gen(board, &Globals::tt);
        Move best_move_this_iter = INVALID_MOVE;
        tt_flag = ALPHA;
        Move move;
        bool moves_available = false;

        // iterate over available moves
        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            moves_available = true;
            std::vector<Move> line;
            Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
            score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, line);
            board.UnmakeMove(move);

            if (!Run()) {
                return 0;
            } else if (score > alpha) {
                if (score >= beta) {
                    Globals::tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
                    return beta;
                }
                pv_line.clear();
                pv_line.emplace_back(move);
                pv_line.insert(pv_line.begin() + 1, line.begin(), line.end());
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move_this_iter = move;
            }
        }

        if (!moves_available) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        }

        if(tt_flag == EXACT_SCORE){
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
            Globals::seldepth.store(std::max(ply, Globals::seldepth.load(std::memory_order_relaxed)),
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

        MoveGen move_gen(board, &Globals::tt);
        Move move;
        // iterate over all available captures
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            Globals::nodes_explored.fetch_add(1, std::memory_order_relaxed);
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

    void SearchThread::BackupPv(std::vector<Move> &pv_line, Board &b, Depth depth){
        Move m = Globals::pvt.ProbePv(b.GetZobristHash());
        if(m != INVALID_MOVE && depth > 0){
            pv_line.emplace_back(m);
            b.MakeMove(m);
            BackupPv(pv_line, b, depth);
            b.UnmakeMove(m);
        }
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

        std::stringstream ss;
        auto elapsed_ms = ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(Globals::nodes_explored.load(std::memory_order_relaxed)) /
                  static_cast<double>(elapsed_ms))) * 1000.0);
        auto pvs_to_send = std::min(static_cast<size_t>(Globals::multi_pv), root_moves.size());

        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info";
            if (pvs_to_send > 1) ss << " multipv " << i + 1;
            ss << " depth " << static_cast<int>(root_moves[i].depth)
               << " seldepth " << static_cast<int>(root_moves[i].seldepth)
               << " nodes " << Globals::nodes_explored.load(std::memory_order_relaxed)
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(Globals::tt.Usage() * 1000)
               << " score ";

            Score score = root_moves[i].score;
            Move move = root_moves[i].move;
            if (score > MIN_MATE_EVAL) {
                int distance_to_mate = static_cast<int>(MATE_SCORE - score);
                ss << "mate " << (distance_to_mate) / 2;
            } else if (score < -MIN_MATE_EVAL) {
                int distance_to_mate = static_cast<int>(MATE_SCORE + score);
                ss << "mate " << -(distance_to_mate) / 2;
            } else {
                ss << "cp " << score;
            }

            ss << " pv " << GetMoveName(move);
            for (Move pv_move : root_moves[i].pv) {
                ss << ' ' << GetMoveName(pv_move);
            }
            if (i + 1 < pvs_to_send) {
                ss << '\n';
            }
        }

        return ss.str();
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