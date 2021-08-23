#include <sstream>
#include "SearchThread.h"
#include "Uci.h"
#include "MoveGen.h"
#include "Search.h"

namespace Meetra {

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (depth_reached = 2;
             depth_reached <= Search::Globals::settings.max_allowed_depth && Search::Run(); depth_reached++) {

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;

            // seldepth_reached is always at least the current depth being searched
            seldepth_reached = depth_reached;

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && depth_reached <= Search::Globals::mt_depth.load(std::memory_order_relaxed)) {
                depth_reached = Search::Globals::mt_depth.load(std::memory_order_relaxed);
            }

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = depth_reached;

                if (IsMainThread() && Search::Globals::show_currmove && Search::ElapsedTimeMs() > 1000) {
                    Uci::SendToGui(GetCurrMoveInfo());
                }

                // if main thread already finished active this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && depth_reached < Search::Globals::mt_depth.load(std::memory_order_relaxed)) {
                    break;
                }

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                curr_rm->nodes++;
                board.MakeMove(curr_rm->move);
                Score score = -NegaMax(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
                board.UnmakeMove(curr_rm->move);

                if (!Search::Run()) {
                    break;
                }

                curr_rm->previous_score = curr_rm->score;
                curr_rm->depth = depth_reached;

                if (Search::Globals::multi_pv > 1) {
                    curr_rm->score = score;
                } else if (score > alpha) {
                    curr_rm->score = score;
                    alpha = score;
                } else {
                    curr_rm->score = NEGATIVE_INF;
                }
            } // end alpha beta loop

            // sort based on score -> previous score -> node count
            std::stable_sort(root_moves.begin(), root_moves.end());

            // checking time and updating GUI when main thread finishes active depth
            if (IsMainThread()) {

                Search::Globals::mt_depth.store(depth_reached, std::memory_order_relaxed);

                // active if we don't have enough time left for a deeper search or mate has been found, and we are not
                // performing fixed time/depth/infinite or multipv search
                if (!Search::Run() || !Search::EnoughTimeLeft() ||
                    (MateInHorizon() && Search::Globals::multi_pv == 1 && !Search::Globals::settings.infinite &&
                     !Search::Globals::settings.fixed_timer)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (depth_reached > Search::Globals::plies_muted) {
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

        if (IsMainThread()) {
            CheckTimers();
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

        while ((move = move_gen.GetBestMove<MoveGen::NORMAL>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            moves_available = true;
            std::vector<Move> line;
            nodes_explored.fetch_add(1, std::memory_order_relaxed);
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

        // update seldepth_reached for this root move and max seldepth_reached for this thread
        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

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
        while ((move = move_gen.GetBestMove<MoveGen::QSEARCH>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
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
            if (depth_reached > distance_to_mate) {
                return true;
            }
        }
        return false;
    }

    bool SearchThread::MateFound() const {
        return root_moves[0].score != NEGATIVE_INF && std::abs(root_moves[0].score) > MIN_MATE_EVAL;
    }

    Search::RootMove SearchThread::GetBestRootMove() const {
        return root_moves[0];
    }

    std::string SearchThread::GetUpdateSearchInfo() const {

        uint64_t total_nodes = 0;
        for (const auto &t : Search::Globals::search_threads) {
            total_nodes += t->NodesExplored();
        }

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(total_nodes) / static_cast<double>(elapsed_ms))) * 1000.0);

        std::ostringstream oss;

        oss << "info depth " << static_cast<int>(depth_reached)
            << " seldepth " << static_cast<int>(seldepth_reached)
            << " nodes " << total_nodes
            << " time " << elapsed_ms
            << " nps " << nps
            << " hashfull " << static_cast<int>(Search::Globals::tt.Usage() * 1000.0);

        return oss.str();
    }

    std::string SearchThread::GetSearchInfo() const {

        uint64_t total_nodes = 0;
        for (const auto &t : Search::Globals::search_threads) {
            total_nodes += t->NodesExplored();
        }

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(total_nodes) / static_cast<double>(elapsed_ms))) * 1000.0);
        auto pvs_to_send = std::min(static_cast<size_t>(Search::Globals::multi_pv), root_moves.size());

        std::ostringstream oss;

        for (auto i = 0; i < pvs_to_send; i++) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << i + 1;
            oss << " depth " << static_cast<int>(root_moves[i].depth)
                << " seldepth " << static_cast<int>(root_moves[i].seldepth)
                << " nodes " << total_nodes
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

    std::string SearchThread::GetCurrMoveInfo() const {
        return "info currmove " + GetMoveName(curr_rm->move) + " currmovenumber " + std::to_string(curr_rm_num + 1);
    }

    std::string SearchThread::GetCurrLineInfo() const {
        std::string ret = "info currline " + GetMoveName(curr_rm->move);
        for (Move pv_move : curr_rm->pv) {
            ret += ' ' + GetMoveName(pv_move);
        }
        return ret;
    }

    void SearchThread::CheckTimers() {

        if ((nodes_explored.load(std::memory_order_relaxed) & 8191) != 0) {
            return;
        }

        using namespace Search;

        auto elapsed_time = ElapsedTimeMs();

        if (!Globals::settings.infinite && Globals::settings.allowed_time < elapsed_time) {
            StopSearch();
        } else if (Globals::last_update_time + UPDATE_INFO_INTERVAL < elapsed_time) {
            Globals::last_update_time = elapsed_time;
            Uci::SendToGui(GetUpdateSearchInfo());
            if (Globals::show_currline) {
                Uci::SendToGui(GetCurrLineInfo());
            }
        }
    }

    SearchThread::SearchThread() {
        id = threads_n++;
        active = false;
        thread = std::jthread([&](const std::stop_token &stop_token) {
            while (true) {
                {
                    std::unique_lock lock(mtx);
                    cond_var.wait(lock, stop_token, [&] { return IsThreadSearching(); });
                }
                if (stop_token.stop_requested()) { return; }
                Search();
            }
        });
    }

    SearchThread::~SearchThread() {
        if (id == 0) {
            threads_n = 0;
        }
        Shutdown();
    }

    void SearchThread::InitNewSearch(Board b, std::vector<Search::RootMove> moves) {
        board = b;
        root_moves = std::move(moves);
        curr_rm = &root_moves[0];
        curr_rm_num = 0;
        depth_reached = 0;
        seldepth_reached = 0;
        nodes_explored = 0;
    }

    void SearchThread::Shutdown() {
        if (thread.joinable()) {
            {
                std::scoped_lock lock(mtx);
                active = false;
            }
            thread.request_stop();
            thread.join();
        }
    }

    void SearchThread::StartThread() {
        {
            std::scoped_lock lock(mtx);
            active = true;
        }
        cond_var.notify_one();
    }
}