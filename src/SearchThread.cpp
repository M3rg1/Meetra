#include <sstream>
#include "SearchThread.h"
#include "Uci.h"
#include "MoveGen.h"
#include "Search.h"
#include <algorithm>
#include <numeric>

namespace Meetra {

    bool IsMateScore(Score s) {
        return s != NEGATIVE_INF && std::abs(s) > MIN_MATE_EVAL;
    }

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (depth_reached = 2;
             depth_reached <= Search::Globals::settings.max_allowed_depth && Search::Run(); depth_reached++) {

            // seldepth_reached is always at least the current depth being searched
            seldepth_reached = depth_reached;

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && depth_reached <= Search::Globals::mt_depth.load(std::memory_order_acquire)) {
                depth_reached = Search::Globals::mt_depth.load(std::memory_order_acquire) + id;
            }

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            bool search_pv = true;

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = depth_reached;

                if (IsMainThread() && Search::Globals::show_currmove && Search::ElapsedTimeMs() > 1000) {
                    Uci::Send(GetCurrMoveInfo());
                }

                // if main thread already finished this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && depth_reached < Search::Globals::mt_depth.load(std::memory_order_acquire)) {
                    break;
                }

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                curr_rm->nodes++;
                board.MakeMove(curr_rm->move);

                Score score;
                if (search_pv) {
                    score = -NegaMax(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
                } else {
                    score = -NegaMax(-alpha - 1, -alpha, depth_reached - 1, 2, curr_rm->pv);
                    if (score > alpha) {
                        score = -NegaMax(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
                    }
                }

                board.UnmakeMove(curr_rm->move);

                if (!Search::Run()) {
                    break;
                }

                curr_rm->previous_score = curr_rm->score;
                curr_rm->depth = depth_reached;

                if (Search::Globals::multi_pv > 1) {
                    curr_rm->score = score;
                } else if (score > alpha) {
                    search_pv = false;
                    curr_rm->score = score;
                    alpha = score;
                } else {
                    curr_rm->score = NEGATIVE_INF;
                }
            } // end alpha beta loop

            // sort based on score -> previous score -> node count
            std::ranges::stable_sort(root_moves);

            // checking time and updating GUI when main thread finishes depth
            if (IsMainThread()) {

                Search::Globals::mt_depth.store(depth_reached, std::memory_order_relaxed);

                // finish if we don't have enough time left for a deeper search or mate has been found, and we are not
                // performing fixed time/depth/infinite or multipv search
                if (!Search::Run() || !Search::EnoughTimeLeft() ||
                    (IsMateScore(root_moves[0].score) && Search::Globals::multi_pv == 1 &&
                     !Search::Globals::settings.infinite && !Search::Globals::settings.fixed_time &&
                     !Search::Globals::settings.fixed_depth)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (depth_reached > Search::Globals::plies_muted) {
                    Uci::Send(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        active = false;
        if (IsMainThread()) {
            Search::FinishSearch();
        }
    }

    Score SearchThread::NegaMax(Score alpha, Score beta, Depth depth, Depth ply, Search::PVMoveLine &pv_line) {

        if (IsMainThread()) {
            CheckTimers();
        }

        if (board.Move50Rule()) {
            // it could be a checkmate on the 50th move
            MoveGen mg(board);
            if (mg.GetAnyMove() == ZERO_MOVE) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        } else if (board.IsRepetition()) {
            return -DRAW_SCORE;
        } else if (depth == 0) {
            return QSearch(alpha, beta, ply);
        }

        TTFlag tt_flag;
        Move tt_move;
        Score score;
        MoveGen move_gen(board);
        Search::Globals::tt.Probe(board.GetHash(), alpha, beta, depth, ply, score, tt_flag, tt_move);

        // do a check of the retrieved move, if it's legal to play in the current position and not corrupted,
        // chances are, the score is correct as well
        if (move_gen.IsPseudoLegal(tt_move)) {
            if (tt_flag == ALPHA || tt_flag == BETA) {
                return score;
            }
            // no cutoff, but we got some move from TT, we will play it as the first move in the main negamax loop
            if (tt_move != ZERO_MOVE) {
                move_gen.PutTTMove(tt_move);
            }
        }

        // https://www.chessprogramming.org/Reverse_Futility_Pruning
        if (depth < 6 && tt_flag != EXACT_SCORE && !move_gen.IsInCheck() && !IsMateScore(beta) &&
            !IsMateScore(alpha)) {
            Score static_score = board.GetEval();
            Score score_margin = 100 * depth;
            if (static_score - score_margin >= beta) {
                return beta; //static_score - score_margin;
            }
        }

        Score best_score = NEGATIVE_INF;
        Move best_move;
        Move move;
        Search::PVMoveLine line;
        tt_flag = ALPHA;
        bool no_moves = true;
        bool search_pv = true;

        bool used_tt_move = tt_move == ZERO_MOVE;

        while ((move = move_gen.GetBestMove<NORMAL>())) {

            // temporary fix to not play TT move twice, this should be fixed in the generator before evaluating the move
            if (move == tt_move) {
                if (used_tt_move) {
                    continue;
                } else {
                    used_tt_move = true;
                }
            }

            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            no_moves = false;
            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
            if (search_pv) {
                score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, line);
            } else {
                score = -NegaMax(-alpha - 1, -alpha, depth - 1, ply + 1, line);
                if (score > alpha && score < beta) {
                    score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, line);
                }
            }

            board.UnmakeMove(move);

            if (!Search::Run()) {
                return 0;
            }

            if (score > best_score) {

                best_score = score;
                best_move = move;

                if (score > alpha) {

                    pv_line.Clear();
                    pv_line.PutMove(move);
                    pv_line.PutLine(line);

                    if (score >= beta) {
                        Search::Globals::tt.Save(board.GetHash(), beta, depth, move, BETA, ply);
                        return beta;
                    }

                    tt_flag = EXACT_SCORE;
                    alpha = score;
                    search_pv = false;
                }
            }
        }

        if (no_moves) {
            return move_gen.IsInCheck() ? -MATE_SCORE + ply : -DRAW_SCORE;
        }

        Search::Globals::tt.Save(board.GetHash(), alpha, depth, best_move, tt_flag, ply);

        return alpha;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

        auto score = board.GetEval();

        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
        }

        MoveGen move_gen(board);

        /*if (ply > max_qsearch_ply && !move_gen.IsInCheck()) {
            return score;
        }*/

        Move move;
        while ((move = move_gen.GetBestMove<QSEARCH>())) {
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

    Search::RootMove SearchThread::GetBestRootMove() const {
        return root_moves[0];
    }

    std::string SearchThread::GetBestRmName() const {
        return board.MoveToName(root_moves[0].move);
    }

    std::string SearchThread::GetUpdateSearchInfo() const {

        uint64_t total_nodes = std::accumulate(Search::Globals::search_threads.begin(),
                                               Search::Globals::search_threads.end(),
                                               0ULL,
                                               [&](auto sum, auto const &t) { return sum + t->NodesExplored(); });

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

        uint64_t total_nodes = std::accumulate(Search::Globals::search_threads.begin(),
                                               Search::Globals::search_threads.end(),
                                               0ULL,
                                               [&](auto sum, auto const &t) { return sum + t->NodesExplored(); });

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(total_nodes) / static_cast<double>(elapsed_ms))) * 1000.0);
        auto pvs_to_send = std::min(static_cast<size_t>(Search::Globals::multi_pv), root_moves.size());

        std::ostringstream oss;

        for (size_t i = 0; i < pvs_to_send; i++) {
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

            oss << " pv " << board.MoveToName(move);
            for (size_t j = 0; j < root_moves[i].pv.Size(); j++) {
                oss << ' ' << board.MoveToName(root_moves[i].pv.At(j));
            }
            if (i + 1 < pvs_to_send) {
                oss << '\n';
            }
        }

        return oss.str();
    }

    std::string SearchThread::GetCurrMoveInfo() const {
        return "info currmove " + board.MoveToName(curr_rm->move) + " currmovenumber " +
               std::to_string(curr_rm_num + 1);
    }

    std::string SearchThread::GetCurrLineInfo() const {
        std::string ret = "info currline " + board.MoveToName(curr_rm->move);
        for (size_t i = 0; i < curr_rm->pv.Size(); i++) {
            ret += ' ' + board.MoveToName(curr_rm->pv.At(i));
        }
        return ret;
    }

    void SearchThread::CheckTimers() {

        if ((nodes_explored.load(std::memory_order_relaxed) & 8191) != 0) {
            return;
        }

        using namespace Search;

        auto elapsed_time = ElapsedTimeMs();

        if (!Globals::settings.infinite && !Globals::settings.fixed_depth &&
            Globals::settings.allowed_time < elapsed_time) {
            StopSearch();
        } else if (Globals::last_update_time + UPDATE_INFO_INTERVAL < elapsed_time) {
            Globals::last_update_time = elapsed_time;
            Uci::Send(GetUpdateSearchInfo());
            if (Globals::show_currline) {
                Uci::Send(GetCurrLineInfo());
            }
        }
    }

    SearchThread::SearchThread() : active(false) {
        id = next_id++;
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
            next_id = 0;
        }
        Shutdown();
    }

    void SearchThread::InitNewSearch(const Board &b, const std::vector<Search::RootMove> &moves) {
        board = b;
        root_moves = moves;
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

    bool SearchThread::DidBeatMove(const Search::RootMove &other) const {
        auto rm = std::ranges::find(root_moves, other);
        if (rm->depth < other.depth) {
            return false;
        }
        if (rm->depth > other.depth) {
            return true;
        }
        if (rm->depth == other.depth) {
            if (GetBestRootMove().move != other.move) {
                return true;
            }
        }
        return false;
    }
}