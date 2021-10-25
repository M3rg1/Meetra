#include <sstream>
#include "SearchThread.h"
#include "Uci.h"
#include "MoveGen.h"
#include "Search.h"
#include <algorithm>

namespace Search {

    bool IsMateScore(Score s) {
        return std::abs(s) <= MATE_SCORE && std::abs(s) >= MIN_MATE_EVAL;
    }

    // the main search function - root position AB search, iterative deepening framework
    void SearchThread::Search() {

        // iterative deepening
        for (depth_reached = 1; depth_reached <= settings.allowed_depth && Run(); ++depth_reached) {

            // seldepth is always at least the current depth being searched
            seldepth_reached = depth_reached;

            // if a helper thread falls behind the main thread, skip current depth and go deeper
            if (!IsMainThread() && depth_reached <= mt_depth) {
                depth_reached = std::min(mt_depth + id, settings.allowed_depth);
            }

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            Score score;

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < root_moves.size(); ++curr_rm_num) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = depth_reached;

                if (IsMainThread() && show_currmove && Time::ElapsedTime<Time::ms>(start_time) > CURRMOVE_DELAY) {
                    Uci::Send(GetCurrMoveInfo());
                }

                // if main thread already finished this depth, there's no reason for a helper thread to remain
                if (!IsMainThread() && depth_reached < mt_depth) {
                    break;
                }

                board.MakeMove(curr_rm->move);

                if (curr_rm_num > 0) {
                    score = -ABSearch<NONPV>(-alpha - 1, -alpha, depth_reached - 1, 2, curr_rm->pv);
                }

                if (curr_rm_num == 0 || (score > alpha && score < beta)) {
                    score = -ABSearch<PV>(-beta, -alpha, depth_reached - 1, 2, curr_rm->pv);
                }

                board.UnmakeMove(curr_rm->move);

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                ++curr_rm->nodes;

                if (!Run()) {
                    break;
                }

                curr_rm->previous_score = curr_rm->score;
                curr_rm->depth = depth_reached;

                if (multi_pv > 1) {
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

            if (IsMainThread() && Run()) {

                mt_depth = depth_reached;

                if (!EnoughTimeLeft()) {
                    break;
                }

                if (depth_reached > plies_muted && depth_reached < settings.allowed_depth) {
                    Uci::Send(GetSearchInfo());
                }
            }
        } // end iterative deepening loop

        if (IsMainThread()) {
            FinishSearch();
        }
    }

    template<Node NodeType>
    Score SearchThread::ABSearch(Score alpha, Score beta, Depth depth, Depth ply, PVLine &pv_line) {

        if (IsMainThread()) {
            CheckTimers();
        }

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

        if (board.Move50Rule()) {
            // it could be a checkmate on the 50th move
            MoveGen mg(board);
            Move m;
            while((m  = mg.GetAnyMove())) {
                if (board.IsMoveLegal(m)) {
                    return -DRAW_SCORE;
                }
            }
            return -MATE_SCORE + ply;
        } else if (board.IsRepetition()) {
            return -DRAW_SCORE;
        } else if (depth <= 0) {
            return QSearch(alpha, beta, ply);
        }

        MoveGen move_gen(board, killers[ply]);
        Score static_eval = board.GetEval();
        Score eval = static_eval;
        Move tt_move;
        Score tt_score;
        TTFlag tt_flag = tt.Probe(board.GetHash(), alpha, beta, depth, ply, tt_score, tt_move);

        if (tt_flag != NOT_FOUND && move_gen.IsPseudoLegal(tt_move)) {

            // always re-search PV nodes
            if (NodeType != PV && tt_flag & CUTOFF) {
                return tt_score;
            }

            move_gen.PutTTMove(tt_move);

            // improve static eval if possible
            if ((tt_flag & ALPHA && eval > tt_score) || (tt_flag & BETA && eval < tt_score)) {
                eval = tt_score;
            }
        }

        killers[ply + 1][0] = ZERO_MOVE;
        killers[ply + 1][1] = ZERO_MOVE;

        bool prune = !move_gen.IsInCheck() && !IsMateScore(beta) && !IsMateScore(alpha) && !IsMateScore(eval);

        // reverse futility pruning
        if (NodeType == NONPV && prune && depth <= FUTILITY_DEPTH) {
            Score futility_score = eval - FUTILITY_FACTOR * depth;
            if (futility_score >= beta) {
                return futility_score;
            }
        }

        PVLine line;
        // null move pruning
        if (NodeType == NONPV && prune && depth >= NULL_DEPTH && eval >= beta && eval >= static_eval) {
            board.MakeNullMove();
            Score null_score = -ABSearch<NULLMOVE>(-beta, -beta + 1, depth - NULL_DEPTH, ply + NULL_DEPTH, line);
            board.UnmakeNullMove();
            if (null_score >= beta) {
                Score verification = ABSearch<NULLMOVE>(beta - 1, beta, depth - NULL_DEPTH, ply + NULL_DEPTH, line);
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

        while ((move = move_gen.GetBestMove<NORMAL>())) {

            // temporary fix to not play TT move twice - this should be done in the move list before evaluating the move
            if (move == tt_move && moves_searched > 0) {
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
            ++curr_rm->nodes;
            ++moves_searched;

            if (!Run()) {
                return 0;
            }

            if (score > best_score) {
                if (score > alpha) {
                    if (score >= beta) {
                        if (killers[ply][0] != move && move_gen.IsQuiet(move)) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = move;
                        }
                        tt.Save(board.GetHash(), score, depth, move, BETA, ply);
                        return score;
                    }
                    pv_line.Clear();
                    pv_line.PutMove(move);
                    pv_line.PutLine(line);
                    tt_flag = EXACT;
                    alpha = score;
                }
                best_score = score;
                best_move = move;
            }
        }

        if (moves_searched == 0) {
            return move_gen.IsInCheck() ? -MATE_SCORE + ply : -DRAW_SCORE;
        }

        tt.Save(board.GetHash(), best_score, depth, best_move, tt_flag, ply);

        return best_score;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        seldepth_reached = std::max(ply, seldepth_reached);

        MoveGen move_gen(board);

        if (!move_gen.IsInCheck()) {
            Score static_eval = board.GetEval();
            if (static_eval > alpha) {
                if (static_eval >= beta) {
                    return static_eval;
                }
                alpha = static_eval;
            }
        }

        Move move;
        Score best_score = NEGATIVE_INF;

        while ((move = move_gen.GetBestMove<QSEARCH>())) {

            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            Score score = -QSearch(-beta, -alpha, ply + 1);

            board.UnmakeMove(move);

            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            ++curr_rm->nodes;

            if (score > best_score) {
                if (score > alpha) {
                    if (score >= beta) {
                        return score;
                    }
                    alpha = score;
                }
                best_score = score;
            }
        }

        if (best_score == NEGATIVE_INF) {
            if (move_gen.IsInCheck()) {
                return -MATE_SCORE + ply;
            }
            best_score = alpha;
        }

        return best_score;
    }

    bool SearchThread::DidBeatMove(const RootMove &other) const {
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

    RootMove SearchThread::GetBestRootMove() const {
        return root_moves[0];
    }

    std::string SearchThread::GetBestRmName() const {
        return board.MoveToName(GetBestRootMove().move);
    }

    std::string SearchThread::GetUpdateSearchInfo() const {

        auto nodes = NodesTotal();
        auto elapsed_ns = Time::ElapsedTime<Time::ns>(start_time);
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);

        std::ostringstream oss;

        oss << "info depth " << depth_reached
            << " seldepth " << seldepth_reached
            << " nodes " << nodes
            << " time " << elapsed_ms
            << " nps " << nps
            << " hashfull " << static_cast<int>(tt.Usage() * 1000.0);

        return oss.str();
    }

    std::string SearchThread::GetSearchInfo() const {

        auto nodes = NodesTotal();
        auto elapsed_ns = Time::ElapsedTime<Time::ns>(start_time);
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);
        auto pvs_to_send = std::min(multi_pv, root_moves.size());

        std::ostringstream oss;

        for (size_t i = 0; i < pvs_to_send; ++i) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << i + 1;
            oss << " depth " << root_moves[i].depth
                << " seldepth " << root_moves[i].seldepth
                << " nodes " << nodes
                << " time " << elapsed_ms
                << " nps " << nps
                << " hashfull " << static_cast<int>(tt.Usage() * 1000.0)
                << " score ";

            Score score = root_moves[i].score;
            if (score > MIN_MATE_EVAL) {
                int distance_to_mate = MATE_SCORE - score;
                oss << "mate " << (distance_to_mate) / 2;
            } else if (score < -MIN_MATE_EVAL) {
                int distance_to_mate = MATE_SCORE + score;
                oss << "mate " << -(distance_to_mate) / 2;
            } else {
                oss << "cp " << score;
            }

            oss << " pv " << board.MoveToName(root_moves[i].move);
            for (size_t j = 0; j < root_moves[i].pv.Size(); ++j) {
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
        for (size_t i = 0; i < curr_rm->pv.Size(); ++i) {
            ret += ' ' + board.MoveToName(curr_rm->pv.At(i));
        }
        return ret;
    }

    void SearchThread::CheckTimers() {

        if ((Nodes() & 4095) != 0) {
            return;
        }

        auto elapsed = Time::ElapsedTime<Time::ms>(start_time);

        if (settings.allowed_time < elapsed || NodesTotal() > settings.allowed_nodes) {
            StopSearch();
        } else if (depth_reached > plies_muted && last_update_time + UPDATE_INFO_INTERVAL < elapsed) {
            last_update_time = elapsed;
            Uci::Send(GetUpdateSearchInfo());
            if (show_currline) {
                Uci::Send(GetCurrLineInfo());
            }
        }
    }

    SearchThread::SearchThread(int id) :
            id(id),
            active(true),
            thread([&](const std::stop_token &stop_token) { InitThread(stop_token); }) {}

    void SearchThread::InitThread(const std::stop_token &stop_token) {
        while (true) {
            {
                std::unique_lock lock(mtx);
                active = false;
                cond_var.notify_all();
                cond_var.wait(lock, stop_token, [&] { return active; });
            }
            if (stop_token.stop_requested()) { return; }
            Search();
        }
    }

    void SearchThread::WaitForFinish() {
        std::unique_lock lock(mtx);
        cond_var.wait(lock, [&] { return !active; });
    }

    void SearchThread::InitNewSearch(const Board &b, const std::vector<RootMove> &moves) {
        board = b;
        root_moves = moves;
        curr_rm = &root_moves[0];
        curr_rm_num = 0;
        depth_reached = 0;
        seldepth_reached = 0;
        nodes_explored = 0;
        for (auto &e: killers) {
            e[0] = ZERO_MOVE;
            e[1] = ZERO_MOVE;
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