#include <sstream>
#include "SearchThread.h"
#include "Uci.h"
#include "MoveGen.h"
#include "Search.h"
#include <algorithm>
#include <numeric>

namespace Meetra {

    bool IsMateScore(Score s) {
        return std::abs(s) <= MATE_SCORE && std::abs(s) >= MIN_MATE_EVAL;
    }

    // the main search function, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (depth_reached = 2; depth_reached <= Search::settings.allowed_depth && Search::Run(); depth_reached++) {

            // seldepth_reached is always at least the current depth being searched
            seldepth_reached = depth_reached;

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && depth_reached <= Search::mt_depth.load(std::memory_order_acquire)) {
                depth_reached = Search::mt_depth.load(std::memory_order_acquire) + id;
            }

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            int moves_searched = 0;

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = depth_reached;

                if (IsMainThread() && Search::show_currmove && Search::ElapsedTimeMs() > 1000) {
                    Uci::Send(GetCurrMoveInfo());
                }

                // if main thread already finished this depth, there's no reason for helper thread to remain
                if (!IsMainThread() && depth_reached < Search::mt_depth.load(std::memory_order_acquire)) {
                    break;
                }

                board.MakeMove(curr_rm->move);

                Score score;

                if (moves_searched > 0) {
                    score = -NegaMax<NONPV>(-alpha - 1, -alpha, depth_reached - 1, 2, curr_rm->pv);
                }

                if (moves_searched == 0 || (score > alpha && score < beta)) {
                    score = -NegaMax<PV>(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
                }

                board.UnmakeMove(curr_rm->move);

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                curr_rm->nodes++;
                moves_searched++;

                if (!Search::Run()) {
                    break;
                }

                curr_rm->previous_score = curr_rm->score;
                curr_rm->depth = depth_reached;

                if (Search::multi_pv > 1) {
                    curr_rm->score = score;
                } else if (score > alpha) {
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

                Search::mt_depth.store(depth_reached, std::memory_order_relaxed);

                // finish if we don't have enough time left for a deeper search or mate has been found, and we are not
                // performing fixed time/depth/infinite or multipv search
                if (!Search::Run() || !Search::EnoughTimeLeft() ||
                    (IsMateScore(root_moves[0].score) && Search::multi_pv == 1 && !Search::settings.infinite &&
                     !Search::settings.fixed_time && !Search::settings.fixed_depth)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (depth_reached > Search::plies_muted) {
                    Uci::Send(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        active = false;
        if (IsMainThread()) {
            Search::FinishSearch();
        }
    }

    template<Node NodeType>
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
        } else if (depth <= 0) {
            return QSearch(alpha, beta, ply);
        }

        TTFlag tt_flag;
        Move tt_move;
        Score score = POSITIVE_INF;
        MoveGen move_gen(board);
        Search::tt.Probe(board.GetHash(), alpha, beta, depth, ply, score, tt_flag, tt_move);

        // checking for data race corruption or hash collision
        if (move_gen.IsPseudoLegal(tt_move)) {
            if (NodeType != PV && tt_flag != NOT_FOUND) {
                return score;
            }
            if (tt_move != ZERO_MOVE) {
                move_gen.PutTTMove(tt_move);
            }
        }

        // best eval available for this position - either hash score or static eval
        Score eval = score == POSITIVE_INF ? board.GetEval() : score;

        // reverse futility pruning
        if (NodeType == NONPV && depth < 6 && !move_gen.IsInCheck() && !IsMateScore(beta) && !IsMateScore(alpha)) {
            Score score_margin = 100 * depth;
            if (eval - score_margin >= beta) {
                return beta; //static_score - score_margin;
            }
        }

        Search::PVMoveLine line;

        // null move pruning
        if (NodeType == NONPV && depth >= 4 && !move_gen.IsInCheck() && eval >= beta && eval >= board.GetEval()) {
            board.MakeNullMove();
            score = -NegaMax<NULLMOVE>(-beta, -beta + 1, depth - 4, ply + 4, line);
            board.UnmakeNullMove();
            if (score >= beta) {
                score = NegaMax<NULLMOVE>(beta - 1, beta, depth - 4, ply + 4, line);
                if (score >= beta && !IsMateScore(score)) {
                    return beta;
                }
            }
        }

        Score best_score = NEGATIVE_INF;
        Move best_move;
        Move move;
        tt_flag = ALPHA;
        int moves_searched = 0;

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

            line.Clear();

            if (NodeType != PV || moves_searched > 0) {
                score = -NegaMax<NONPV>(-alpha - 1, -alpha, depth - 1, ply + 1, line);
            }

            if (NodeType == PV && (moves_searched == 0 || (score > alpha && score < beta))) {
                score = -NegaMax<PV>(-beta, -alpha, depth - 1, ply + 1, line);
            }

            board.UnmakeMove(move);

            moves_searched++;
            curr_rm->nodes++;
            nodes_explored.fetch_add(1, std::memory_order_relaxed);

            if (!Search::Run()) {
                return 0;
            }

            if (score > best_score) {

                best_score = score;
                best_move = move;

                if (score > alpha) {

                    if (score >= beta) {
                        Search::tt.Save(board.GetHash(), beta, depth, move, BETA, ply);
                        return beta;
                    }

                    pv_line.Clear();
                    pv_line.PutMove(move);
                    pv_line.PutLine(line);

                    tt_flag = EXACT_SCORE;
                    alpha = score;
                }
            }
        }

        if (moves_searched == 0) {
            return move_gen.IsInCheck() ? -MATE_SCORE + ply : -DRAW_SCORE;
        }

        Search::tt.Save(board.GetHash(), alpha, depth, best_move, tt_flag, ply);

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

    Search::RootMove SearchThread::GetBestRootMove() const {
        return root_moves[0];
    }

    std::string SearchThread::GetBestRmName() const {
        return board.MoveToName(root_moves[0].move);
    }

    std::string SearchThread::GetUpdateSearchInfo() const {

        auto nodes = std::accumulate(Search::threads.begin(),
                                     Search::threads.end(),
                                     0ULL,
                                     [&](auto sum, auto const &t) { return sum + t->NodesExplored(); });

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(nodes) / static_cast<double>(elapsed_ms))) * 1000.0);

        std::ostringstream oss;

        oss << "info depth " << depth_reached
            << " seldepth " << seldepth_reached
            << " nodes " << nodes
            << " time " << elapsed_ms
            << " nps " << nps
            << " hashfull " << static_cast<int>(Search::tt.Usage() * 1000.0);

        return oss.str();
    }

    std::string SearchThread::GetSearchInfo() const {

        auto nodes = std::accumulate(Search::threads.begin(),
                                     Search::threads.end(),
                                     0ULL,
                                     [&](auto sum, auto const &t) { return sum + t->NodesExplored(); });

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(((static_cast<double>(nodes) / static_cast<double>(elapsed_ms))) * 1000.0);
        auto pvs_to_send = std::min(Search::multi_pv, static_cast<int>(root_moves.size()));

        std::ostringstream oss;

        for (int i = 0; i < pvs_to_send; i++) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << i + 1;
            oss << " depth " << root_moves[i].depth
                << " seldepth " <<root_moves[i].seldepth
                << " nodes " << nodes
                << " time " << elapsed_ms
                << " nps " << nps
                << " hashfull " << static_cast<int>(Search::tt.Usage() * 1000)
                << " score ";

            Score score = root_moves[i].score;
            Move move = root_moves[i].move;
            if (score > MIN_MATE_EVAL) {
                int distance_to_mate = MATE_SCORE - score;
                oss << "mate " << (distance_to_mate) / 2;
            } else if (score < -MIN_MATE_EVAL) {
                int distance_to_mate = MATE_SCORE + score;
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

        auto elapsed_time = Search::ElapsedTimeMs();

        if (!Search::settings.infinite && !Search::settings.fixed_depth &&
            Search::settings.allowed_time < elapsed_time) {
            Search::StopSearch();
        } else if (Search::last_update_time + UPDATE_INFO_INTERVAL < elapsed_time) {
            Search::last_update_time = elapsed_time;
            Uci::Send(GetUpdateSearchInfo());
            if (Search::show_currline) {
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
                    cond_var.wait(lock, stop_token, [&] { return active.load(); });
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
}