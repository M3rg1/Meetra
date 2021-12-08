#include <sstream>
#include "SearchThread.h"
#include "MoveGen.h"
#include "Search.h"
#include <syncstream>
#include <iostream>

namespace Search {

    // the main search function - root position AB search, iterative deepening framework
    void SearchThread::Search() {

        // iterative deepening
        for (depth_reached = 1; depth_reached <= settings.allowed_depth && Run(); ++depth_reached) {

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
                curr_rm->seldepth = depth_reached; // seldepth is always at least the current depth

                if (IsMainThread() && show_currmove && Time::ElapsedSince<Time::ms>(start_time) > CURRMOVE_DELAY) {
                    SendCurrMoveInfo();
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

                if (!Run()) {
                    break;
                }

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                ++curr_rm->nodes;

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
                    SendFullSearchInfo();
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

        if (board.Move50Rule()) {
            // it could be checkmate (or stalemate) on the 50th move
            MoveGen mg(board);
            bool legal_moves = false;
            while (Move m = mg.GetAnyMove()) {
                if (board.IsMoveLegal(m)) {
                    legal_moves = true;
                    break;
                }
            }
            if (board.IsInCheck() && !legal_moves) {
                return -MATE_SCORE + ply;
            }
            return -DRAW_SCORE;
        } else if (board.IsRepetition()) {
            return -DRAW_SCORE;
        } else if (ply >= MAX_SEARCH_DEPTH) {
            return board.GetEval();
        } else if (depth <= 0) {
            if (NodeType == PV && board.IsInCheck()) {
                depth = 1;
            } else {
                return QSearch(alpha, beta, ply);
            }
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
        } else {
            tt_move = ZERO_MOVE;
        }

        bool prune = !board.IsInCheck();

        // reverse futility pruning
        if (NodeType == NONPV
            && prune
            && depth <= FUTILITY_DEPTH
            && eval - FUTILITY_FACTOR * depth >= beta
                ) {
            return eval;
        }

        std::ranges::fill(killers[ply + 1], ZERO_MOVE);
        PVLine line;

        // null move pruning
        if (NodeType == NONPV
            && prune
            && depth >= NULL_DEPTH
            && eval >= beta
            && eval >= static_eval
                ) {
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

        tt_flag = ALPHA;
        Score best_score = NEGATIVE_INF;
        Move best_move;
        size_t moves_searched = 0;

        while (Move move = move_gen.GetBestMove<NORMAL>()) {

            // temporary fix to not play TT move twice - this should be done in the move list before evaluating the move
            if (move == tt_move && moves_searched > 0) {
                continue;
            }

            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            // late move reduction
            Depth reduction = 0;
            if (NodeType != PV
                && prune
                && depth >= 3
                && moves_searched >= 2
                && !board.IsInCheck()
                && !board.CapturedPiece()
                && !IsPromotion(move)
                    ) {
                if (moves_searched >= 7) {
                    reduction = depth / 3;
                } else {
                    reduction = 1;
                }
            }

            Score score;
            if (NodeType != PV || moves_searched > 0) {
                score = -ABSearch<NONPV>(-alpha - 1, -alpha, depth - 1 - reduction, ply + 1, line);
            }
            if (NodeType == PV && (moves_searched == 0 || (score > alpha && score < beta))) {
                line.Clear();
                score = -ABSearch<PV>(-beta, -alpha, depth - 1, ply + 1, line);
            }

            board.UnmakeMove(move);

            if (!Run()) {
                return 0;
            }

            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            ++curr_rm->nodes;
            ++moves_searched;

            if (score > best_score) {
                if (score > alpha) {
                    if (score >= beta) {
                        UpdateKillers(move, ply);
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
        } // end move loop

        if (moves_searched == 0) {
            return board.IsInCheck() ? -MATE_SCORE + ply : -DRAW_SCORE;
        }

        tt.Save(board.GetHash(), best_score, depth, best_move, tt_flag, ply);

        return best_score;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);

        if (ply >= 3 * depth_reached || ply >= MAX_SEARCH_DEPTH) {
            return board.GetEval();
        }

        if (board.Move50Rule() || board.IsRepetition()) {
            return -DRAW_SCORE;
        }

        if (!board.IsInCheck()) {
            Score static_eval = board.GetEval();
            if (static_eval > alpha) {
                if (static_eval >= beta) {
                    return static_eval;
                }
                alpha = static_eval;
            }
        }

        MoveGen move_gen(board);
        Score best_score = NEGATIVE_INF;
        while (Move move = move_gen.GetBestMove<QSEARCH>()) {

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
            if (board.IsInCheck()) {
                return -MATE_SCORE + ply;
            }
            best_score = alpha;
        }

        return best_score;
    }

    void SearchThread::UpdateKillers(Move move, Depth ply) {
        if (board.IsQuiet(move) && std::ranges::find(killers[ply], move) == std::end(killers[ply])) {
            std::copy_n(killers[ply], KILLER_SLOTS - 1, killers[ply] + 1);
            killers[ply][0] = move;
        }
    }

    bool SearchThread::DidBeatMove(const RootMove &move) const {
        auto rm = std::ranges::find(root_moves, move);
        if (rm == root_moves.end()) {
            return false;
        } else if (rm->depth < move.depth) {
            return false;
        } else if (rm->depth > move.depth) {
            return true;
        } else if (GetBestRootMove().move != move.move) {
            return true;
        }
        return false;
    }

    RootMove SearchThread::GetBestRootMove() const {
        return root_moves[0];
    }

    void SearchThread::SendBestMove() const {
        std::osyncstream(std::cout) << "bestmove " << board.MoveToName(GetBestRootMove().move) << std::endl;
    }

    Depth SearchThread::GetMaxSeldepth() const {
        return std::ranges::max_element(root_moves, [&](const auto &m1, const auto &m2) {
            return m2.depth == depth_reached && m1.seldepth < m2.seldepth;
        })->seldepth;
    }

    void SearchThread::SendBriefSearchInfo() const {

        auto nodes = NodesTotal();
        auto elapsed_ns = Time::ElapsedSince<Time::ns>(start_time) + 1;
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);

        std::osyncstream(std::cout)
                << "info depth " << depth_reached
                << " seldepth " << GetMaxSeldepth()
                << " nodes " << nodes
                << " time " << elapsed_ms
                << " nps " << nps
                << " hashfull " << static_cast<int>(tt.Usage() * 1000.0)
                << std::endl;
    }

    void SearchThread::SendFullSearchInfo() const {

        auto nodes = NodesTotal();
        auto elapsed_ns = Time::ElapsedSince<Time::ns>(start_time) + 1;
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);
        auto pvs_to_send = std::min(multi_pv, root_moves.size());

        std::osyncstream oss(std::cout);
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
            if (score > MIN_MATE_EVAL) oss << "mate " << (MATE_SCORE - score) / 2;
            else if (score < -MIN_MATE_EVAL) oss << "mate " << -(MATE_SCORE + score) / 2;
            else oss << "cp " << score;

            oss << " pv " << board.MoveToName(root_moves[i].move);
            for (size_t j = 0; j < root_moves[i].pv.Size(); ++j) {
                oss << ' ' << board.MoveToName(root_moves[i].pv.At(j));
            }
            if (i + 1 < pvs_to_send) oss << '\n';
        }
        oss << std::endl;
    }

    void SearchThread::SendCurrMoveInfo() const {
        std::osyncstream(std::cout) << "info currmove " << board.MoveToName(curr_rm->move) << " currmovenumber "
                                    << curr_rm_num + 1 << std::endl;
    }

    void SearchThread::SendCurrLineInfo() const {
        std::osyncstream oss(std::cout);
        oss << "info currline " << board.MoveToName(curr_rm->move);
        for (size_t i = 0; i < curr_rm->pv.Size(); ++i) {
            oss << ' ' << board.MoveToName(curr_rm->pv.At(i));
        }
        oss << std::endl;
    }

    void SearchThread::CheckTimers() {

        if ((Nodes() & 4095) != 0) {
            return;
        }

        auto elapsed = Time::ElapsedSince<Time::ms>(start_time);

        if (settings.allowed_time < elapsed || NodesTotal() > settings.allowed_nodes) {
            StopSearch();
        } else if (depth_reached > plies_muted && last_update_time + update_interval < elapsed) {
            last_update_time = elapsed;
            SendBriefSearchInfo();
            if (show_currline) {
                SendCurrLineInfo();
            }
        }
    }

    SearchThread::SearchThread(int id) :
            id(id),
            active(true),
            thread(std::bind_front(&SearchThread::InitThread, this)) {};

    void SearchThread::InitThread(std::stop_token stop_token) {
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
        nodes_explored = 0;
        std::ranges::for_each(killers, [&](auto &k) { std::ranges::fill(k, ZERO_MOVE); });
    }

    void SearchThread::StartThread() {
        {
            std::scoped_lock lock(mtx);
            active = true;
        }
        cond_var.notify_one();
    }
}