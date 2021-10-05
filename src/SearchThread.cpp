#include <sstream>
#include "SearchThread.h"
#include "Uci.h"
#include "MoveGen.h"
#include "Search.h"
#include <algorithm>

namespace Meetra {

    bool IsMateScore(Score s) {
        return std::abs(s) <= MATE_SCORE && std::abs(s) >= MIN_MATE_EVAL;
    }

    // the main search function - root position AB search, iterative deepening framework
    void SearchThread::Search() {

        active = true;

        // iterative deepening
        for (depth_reached = 1; depth_reached <= Search::settings.allowed_depth && Search::Run(); depth_reached++) {

            // seldepth_reached is always at least the current depth being searched
            seldepth_reached = depth_reached;

            // if helper thread falls behind main thread, skip depth and go deeper
            if (!IsMainThread() && depth_reached <= Search::MtDepth()) {
                depth_reached = std::min(Search::MtDepth() + id, Search::settings.allowed_depth);
            }

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            int moves_searched = 0;

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); curr_rm_num++) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = depth_reached;

                if (IsMainThread() && Search::show_currmove && Time::ElapsedTime<Time::ms>(Search::start_time) > 1000) {
                    Uci::Send(GetCurrMoveInfo());
                }

                // if main thread already finished this depth, there's no reason for a helper thread to remain
                if (!IsMainThread() && depth_reached < Search::MtDepth()) {
                    break;
                }

                board.MakeMove(curr_rm->move);

                Score score;

                if (moves_searched > 0) {
                    score = -ABSearch<NONPV>(-alpha - 1, -alpha, depth_reached - 1, 2, curr_rm->pv);
                }

                if (moves_searched == 0 || (score > alpha && score < beta)) {
                    score = -ABSearch<PV>(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
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
                    (IsMateScore(root_moves[0].score) && Search::multi_pv == 1 && !Search::settings.infinite)) {
                    break;
                }

                // update GUI with info about currently finished depth we searched
                if (depth_reached > Search::plies_muted && depth_reached < Search::settings.allowed_depth) {
                    Uci::Send(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        if (IsMainThread()) {
            Search::FinishSearch();
        }

        active = false;
    }

    template<SearchThread::Node NodeType>
    Score SearchThread::ABSearch(Score alpha, Score beta, Depth depth, Depth ply, Search::PVMoveLine &pv_line) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

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

        MoveGen move_gen(board);
        Score static_eval = board.GetEval();
        Score eval = static_eval;
        Move tt_move;
        Score tt_score;
        TTFlag tt_flag = Search::tt.Probe(board.GetHash(), alpha, beta, depth, ply, tt_score, tt_move);

        if (tt_flag != NOT_FOUND && move_gen.IsPseudoLegal(tt_move)) {

            // we always re-search PV nodes
            if (NodeType != PV && tt_flag & CUTOFF) {
                return tt_score;
            }

            // the move wasn't good enough to cause cutoff, but we can still use it to order our moves
            move_gen.PutTTMove(tt_move);

            // improve our static eval if possible
            if ((tt_flag & BETA && eval < tt_score) || (tt_flag & ALPHA && eval > tt_score)) {
                eval = tt_score;
            }
        }

        bool prune = !move_gen.IsInCheck() && !IsMateScore(beta) && !IsMateScore(alpha) && !IsMateScore(eval);

        // reverse futility pruning
        if (NodeType == NONPV && prune && depth < 6) {
            Score score_margin = 100 * depth;
            if (eval - score_margin >= beta) {
                return eval - score_margin;
            }
        }

        Search::PVMoveLine line;
        // null move pruning
        constexpr int R = 4;
        if (NodeType == NONPV && prune && depth >= R && eval >= beta && eval >= static_eval) {
            board.MakeNullMove();
            Score null_score = -ABSearch<NULLMOVE>(-beta, -beta + 1, depth - R, ply + R, line);
            board.UnmakeNullMove();
            if (null_score >= beta) {
                Score verification = ABSearch<NULLMOVE>(beta - 1, beta, depth - R, ply + R, line);
                if (verification >= beta) {
                    return null_score;
                }
            }
        }

        Score score;
        Score best_score = NEGATIVE_INF;
        Move best_move;
        Move move;
        tt_flag = ALPHA;
        int moves_searched = 0;

        while ((move = move_gen.GetBestMove<MoveGen::NORMAL>())) {

            // temporary fix to not play TT move twice - this should be done in the generator before evaluating the move
            if (move == tt_move && moves_searched >= 1) {
                continue;
            }

            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            line.Clear();

            if (NodeType != PV || moves_searched > 0) {
                score = -ABSearch<NONPV>(-alpha - 1, -alpha, depth - 1, ply + 1, line);
            }

            if (NodeType == PV && (moves_searched == 0 || (score > alpha && score < beta))) {
                score = -ABSearch<PV>(-beta, -alpha, depth - 1, ply + 1, line);
            }

            board.UnmakeMove(move);

            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
            moves_searched++;

            if (!Search::Run()) {
                return 0;
            }

            if (score > best_score) {

                best_score = score;
                best_move = move;

                if (score > alpha) {

                    if (score >= beta) {

                        // killers only for pv nodes ?? for nonpv as well?? what about null move search
                        if (NodeType == PV && move_gen.IsCapture(move)) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = move;
                        }

                        Search::tt.Save(board.GetHash(), score, depth, move, BETA, ply);
                        return score;
                    }

                    pv_line.Clear();
                    pv_line.PutMove(move);
                    pv_line.PutLine(line);

                    tt_flag = EXACT;
                    alpha = score;
                }
            }
        }

        if (moves_searched == 0) {
            return move_gen.IsInCheck() ? -MATE_SCORE + ply : -DRAW_SCORE;
        }

        Search::tt.Save(board.GetHash(), best_score, depth, best_move, tt_flag, ply);

        return best_score;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

        auto score = board.GetEval();
        if (score > alpha) {
            if (score >= beta) {
                return score;
            }
            alpha = score;
        }

        MoveGen move_gen(board);
        Move move;
        int moves_searched = 0;

        while ((move = move_gen.GetBestMove<MoveGen::QSEARCH>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            score = -QSearch(-beta, -alpha, ply + 1);
            board.UnmakeMove(move);

            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            curr_rm->nodes++;
            moves_searched++;

            if (score > alpha) {
                if (score >= beta) {
                    return score;
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

        auto nodes = Search::NodesTotal();
        auto elapsed_ns = Time::ElapsedTime<Time::ns>(Search::start_time);
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);

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

        auto nodes = Search::NodesTotal();
        auto elapsed_ns = Time::ElapsedTime<Time::ns>(Search::start_time);
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);
        auto pvs_to_send = std::min(Search::multi_pv, static_cast<int>(root_moves.size()));

        std::ostringstream oss;

        for (int i = 0; i < pvs_to_send; i++) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << i + 1;
            oss << " depth " << root_moves[i].depth
                << " seldepth " << root_moves[i].seldepth
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

        if ((Nodes() & 8191) != 0) {
            return;
        }

        auto elapsed = Time::ElapsedTime<Time::ms>(Search::start_time);

        if (Search::settings.allowed_time < elapsed || Search::NodesTotal() > Search::settings.allowed_nodes) {
            Search::StopSearch();
        } else if (depth_reached > Search::plies_muted && Search::last_update_time + UPDATE_INFO_INTERVAL < elapsed) {
            Search::last_update_time = elapsed;
            Uci::Send(GetUpdateSearchInfo());
            if (Search::show_currline) {
                Uci::Send(GetCurrLineInfo());
            }
        }
    }

    SearchThread::SearchThread() : thread([&](const std::stop_token &stop_token) { InitThread(stop_token); }) {}

    void SearchThread::InitThread(const std::stop_token &stop_token) {
        while (true) {
            {
                std::unique_lock lock(mtx);
                cond_var.wait(lock, stop_token, [&] { return active.load(); });
            }
            if (stop_token.stop_requested()) { return; }
            Search();
        }
    }

    SearchThread::~SearchThread() {
        if (IsMainThread()) {
            next_id = 0;
        }
        if (thread.joinable()) {
            thread.request_stop();
            thread.join();
        }
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

    void SearchThread::StartThread() {
        {
            std::scoped_lock lock(mtx);
            active = true;
        }
        cond_var.notify_one();
    }
}