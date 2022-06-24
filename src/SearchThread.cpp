#include <syncstream>
#include <iostream>
#include <functional>
#include <ranges>
#include "SearchThread.h"
#include "MoveGen.h"
#include "Search.h"

namespace Search {

    // the main search function - root position AB search, iterative deepening framework
    void SearchThread::Search() {

        // iterative deepening
        for (depth_reached = 1; depth_reached <= MAX_SEARCH_DEPTH && Run(); ++depth_reached) {

            // if a helper thread falls behind the main thread, skip current depth and go deeper
            if (!IsMainThread() && depth_reached <= mt_depth) {
                depth_reached = std::min(static_cast<Depth>(mt_depth + id),
                                         settings.limit_depth ? settings.allowed_depth : MAX_SEARCH_DEPTH);
            }

            Score alpha = NEGATIVE_INF;
            Score beta = POSITIVE_INF;
            Score score = 0;

            // TODO zero out node counts for the moves??
            // std::ranges::for_each(root_moves, [](auto &r){ r.nodes = 0; });

            // alpha beta search over root moves
            for (curr_rm_num = 0; curr_rm_num < static_cast<int>(root_moves.size()); ++curr_rm_num) {

                curr_rm = &root_moves[curr_rm_num];
                curr_rm->seldepth = 1;

                if (IsMainThread()
                    && show_currmove
                    && depth_reached > plies_muted
                    && ElapsedSince(start_time) > CURRMOVE_DELAY
                        ) {
                    SendCurrMoveInfo();
                }

                board.MakeMove(curr_rm->move);

                if (curr_rm_num > 0 && multi_pv == 1) {
                    score = -ABSearch<NON_PV>(-alpha - 1, -alpha, depth_reached - 1, 2);
                }
                if (curr_rm_num == 0 || (score > alpha && score < beta)) {
                    pv[2].Clear();
                    score = -ABSearch<PV>(-beta, -alpha, depth_reached - 1, 2);
                }

                board.UnmakeMove(curr_rm->move);

                if (!Run()) {
                    break;
                }

                nodes_explored.fetch_add(1, std::memory_order_relaxed);
                ++curr_rm->nodes;

                curr_rm->previous_score = curr_rm->score;
                curr_rm->depth = depth_reached;

                if (score > alpha) {
                    curr_rm->pv.Clear();
                    curr_rm->pv.Merge(pv[2]);
                    curr_rm->score = score;
                    if (multi_pv == 1) {
                        alpha = score;
                    }
                } else {
                    curr_rm->score = NEGATIVE_INF;
                }

                // if main thread already finished this depth, there's no reason for a helper thread to remain
                if (!IsMainThread() && depth_reached < mt_depth) {
                    break;
                }
            } // end alpha beta loop

            // sort based on score -> previous score -> node count
            std::ranges::sort(root_moves);

            if (DepthLimitReached()) {
                StopSearch();
            }

            if (IsMainThread() && Run()) {
                mt_depth = depth_reached;
                if (!EnoughTimeLeft()) {
                    StopSearch();
                } else if (depth_reached > plies_muted && depth_reached < MAX_SEARCH_DEPTH) {
                    SendFullSearchInfo();
                }
            }

        } // end iterative deepening loop

        if (IsMainThread()) {
            FinalizeSearch();
        }
    }

    template<SearchThread::Node NodeType>
    Score SearchThread::ABSearch(Score alpha, Score beta, Depth depth, Depth ply) {

        if (IsMainThread()) {
            CheckTermination();
        }

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);
        pv[ply].Clear();

        if (board.IsDraw()) {
            return RandomizedDrawScore();
        } else if (depth <= 0) {
            return QSearch(alpha, beta, ply);
        }

        // mate distance pruning
        alpha = std::max(-MATE_SCORE + ply, alpha);
        beta = std::min(MATE_SCORE - (ply + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }

        using TT = TranspositionTable;
        MoveGen move_gen(board, killers[ply]);
        Score static_eval = board.Eval();
        Score eval = static_eval; // eval is not used if we are in check
        auto [tt_flag, tt_score, tt_move] = tt.Probe(board.Hash(), alpha, beta, depth, ply);

        if (tt_flag != TT::NOT_FOUND && move_gen.IsPseudoLegal(tt_move)) {
            if (NodeType != PV && tt_flag & TT::CUTOFF) {
                return tt_score;
            }
            move_gen.PutTTMove(tt_move);
            // improve static eval using stored TT score if possible
            if ((tt_flag & TT::UPPER && eval > tt_score) || (tt_flag & TT::LOWER && eval < tt_score)) {
                eval = tt_score;
            }
        }

        // reverse futility pruning
        if (NodeType == NON_PV
            && !board.IsInCheck()
            && depth <= FUTILITY_MAX_DEPTH
            && eval < MIN_MATE_EVAL
            && eval - FUTILITY_FACTOR * depth >= beta
                ) {
            return eval;
        }

        killers[ply + 1].Clear();

        // null move pruning
        if (NodeType == NON_PV
            && !board.IsInCheck()
            && depth >= NULL_MIN_DEPTH
            && eval >= beta
            && eval >= static_eval
            && beta < MIN_MATE_EVAL
            && alpha > -MIN_MATE_EVAL
                ) {
            Depth R = NULL_BASE_REDUCTION + depth / 5;
            board.MakeNullMove();
            Score null_score = -ABSearch<NULL_MOVE>(-beta, -beta + 1, depth - R, ply + 1);
            board.UnmakeNullMove();
            if (null_score >= beta) {
                return null_score >= MIN_MATE_EVAL ? beta : null_score;
            }
        }

        tt_flag = TT::UPPER;
        Score best_score = NEGATIVE_INF;
        Move best_move;
        int moves_searched = 0;
        bool do_lmr = !board.IsInCheck() && depth >= LMR_MIN_DEPTH;

        while (Move move = move_gen.NextBestMove<MoveGen::NORMAL>()) {

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
            if (do_lmr
                && moves_searched >= LMR_MIN_MOVES_SEARCHED
                && !board.IsInCheck()
                && !board.CapturedPiece()
                && !IsPromotion(move)
                && best_score > -MIN_MATE_EVAL
                    ) {
                if (moves_searched >= 7) reduction += depth / 3;
                if (NodeType != PV) ++reduction;
                if (killers[ply].Contains(move)) --reduction;
                reduction = std::clamp(reduction, 1, depth - 2);
            }

            Score score = NEGATIVE_INF;
            // LMR search: LMR condition has been triggered
            if (reduction > 0) {
                score = -ABSearch<NON_PV>(-alpha - 1, -alpha, depth - reduction - 1, ply + 1);
            }
            // PVS search: LMR search failed. Or LMR wasn't performed, and it's either non PV node or a PV node but not the first move
            if (score > alpha || (reduction == 0 && (NodeType != PV || moves_searched > 0))) {
                score = -ABSearch<NON_PV>(-alpha - 1, -alpha, depth - 1, ply + 1);
            }
            // Full search: PV nodes only. It's either the first move we are searching or PVS search failed
            if (NodeType == PV && (moves_searched == 0 || (score > alpha && score < beta))) {
                score = -ABSearch<PV>(-beta, -alpha, depth - 1, ply + 1);
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
                    if constexpr (NodeType == PV) {
                        pv[ply].Clear();
                        pv[ply].PutMove(move);
                        pv[ply].Merge(pv[ply + 1]);
                    }
                    if (score >= beta) {
                        UpdateKillers(move, ply);
                        tt.Save(board.Hash(), score, depth, move, TT::LOWER, ply);
                        return score;
                    }
                    tt_flag = TT::EXACT;
                    alpha = score;
                }
                best_score = score;
                best_move = move;
            }
        } // end move loop

        if (moves_searched == 0) {
            return board.IsInCheck() ? -MATE_SCORE + ply : RandomizedDrawScore();
        }

        tt.Save(board.Hash(), best_score, depth, best_move, tt_flag, ply);

        return best_score;
    }

    Score SearchThread::QSearch(Score alpha, Score beta, Depth ply) {

        curr_rm->seldepth = std::max(ply, curr_rm->seldepth);

        if (ply >= 3 * depth_reached || ply >= MAX_SEARCH_DEPTH) {
            return board.Eval();
        }

        if (board.IsDraw()) {
            return RandomizedDrawScore();
        }

        Score best_score = NEGATIVE_INF;
        if (!board.IsInCheck()) {
            best_score = board.Eval();
            if (best_score > alpha) {
                if (best_score >= beta) {
                    return best_score;
                }
                alpha = best_score;
            }
        }

        MoveGen move_gen(board);
        auto moves_searched = 0;
        while (Move move = move_gen.NextBestMove<MoveGen::QSEARCH>()) {

            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }

            Score score = -QSearch(-beta, -alpha, ply + 1);

            board.UnmakeMove(move);

            nodes_explored.fetch_add(1, std::memory_order_relaxed);
            ++curr_rm->nodes;
            ++moves_searched;

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

        // since we generate all moves when in check, if there are no moves, it's checkmate
        if (moves_searched == 0 && board.IsInCheck()) {
            return -MATE_SCORE + ply;
        }

        return best_score;
    }

    void SearchThread::UpdateKillers(Move move, Depth ply) {
        // if the move is not present in the killers array, shift all moves one to the right and put the new move first
        if (board.IsQuiet(move) && !killers[ply].Contains(move)) {
            killers[ply].PutMove(move);
        }
    }

    Score SearchThread::RandomizedDrawScore() const {
        return DRAW_SCORE + 1 - static_cast<Score>(Nodes() & 2);
    }

    bool SearchThread::DidBeatMove(const RootMove &move) const {
        const auto rm = std::ranges::find(root_moves, move);
        if (rm != root_moves.end()) {
            if (rm->depth < move.depth) {
                return false;
            } else if (rm->depth > move.depth || BestRootMove().score > move.score) {
                return true;
            }
        }
        return false;
    }

    RootMove SearchThread::BestRootMove() const {
        return root_moves[0];
    }

    void SearchThread::SendBestMove() const {
        std::osyncstream(std::cout) << "bestmove " << board.MoveToStr(BestRootMove().move) << std::endl;
    }

    Depth SearchThread::SeldepthReached() const {
        return std::ranges::max_element(root_moves, [&](const auto &m1, const auto &m2) {
            return m2.depth == depth_reached && m1.seldepth < m2.seldepth;
        })->seldepth;
    }

    void SearchThread::SendBriefSearchInfo() const {

        auto elapsed = ElapsedSince(start_time);
        auto nodes = NodesTotal();

        std::osyncstream(std::cout)
                << "info depth " << depth_reached
                << " seldepth " << SeldepthReached()
                << " nodes " << nodes
                << " time " << elapsed
                << " nps " << Nps(nodes, elapsed)
                << " hashfull " << tt.Hashfull()
                << std::endl;
    }

    void SearchThread::SendFullSearchInfo() const {

        auto elapsed = ElapsedSince(start_time);
        auto nodes = NodesTotal();

        std::osyncstream oss(std::cout);
        auto pvs_to_send = std::min(multi_pv, static_cast<int>(root_moves.size()));
        for (const auto &rm: root_moves | std::views::take(pvs_to_send)) {
            oss << "info";
            if (pvs_to_send > 1) oss << " multipv " << (std::distance(root_moves.data(), &rm) + 1);
            oss << " depth " << rm.depth
                << " seldepth " << rm.seldepth
                << " nodes " << nodes
                << " time " << elapsed
                << " nps " << Nps(nodes, elapsed)
                << " hashfull " << tt.Hashfull()
                << " score ";

            Score score = (rm.score > 1 || rm.score < -1) ? rm.score : 0;
            if (score > MIN_MATE_EVAL) oss << "mate " << (MATE_SCORE - score) / 2;
            else if (score < -MIN_MATE_EVAL) oss << "mate " << -(MATE_SCORE + score) / 2;
            else oss << "cp " << score;

            oss << " pv " << board.MoveToStr(rm.move);
            std::ranges::for_each(rm.pv, [&](auto m) { oss << ' ' << board.MoveToStr(m); });
            oss << '\n';
        }
        oss << std::flush;
    }

    void SearchThread::SendCurrMoveInfo() const {
        std::osyncstream(std::cout)
                << "info currmove " << board.MoveToStr(curr_rm->move)
                << " currmovenumber " << curr_rm_num + 1
                << std::endl;
    }

    void SearchThread::SendCurrLineInfo() const {
        std::osyncstream oss(std::cout);
        oss << "info currline " << board.MoveToStr(curr_rm->move);
        std::ranges::for_each(curr_rm->pv, [&](auto m) { oss << ' ' << board.MoveToStr(m); });
        oss << std::endl;
    }

    void SearchThread::CheckTermination() {

        // make sure to be precise when in node limited search
        if (NodesLimitReached()) {
            StopSearch();
            return;
        }

        // querying time is expensive, don't do it too often
        if ((Nodes() & 4095) != 0) {
            return;
        }

        auto elapsed = ElapsedSince(start_time);

        if ((!IsSearchLimited() && elapsed > time_limit) || MoveTimeLimitReached()) {
            StopSearch();
        } else if (depth_reached > plies_muted && last_update_time + update_interval < elapsed) {
            last_update_time = elapsed;
            SendBriefSearchInfo();
            if (show_currline) {
                SendCurrLineInfo();
            }
        }
    }

    SearchThread::SearchThread(int i) :
            id(i),
            active(true),
            thread(std::bind_front(&SearchThread::InitThread, this)) {}

    void SearchThread::InitThread(std::stop_token stop_token) {
        while (true) {
            {
                std::unique_lock lock(mtx);
                active = false;
                cond_var.notify_one();
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
        std::ranges::for_each(killers, [&](auto &k) { k.Clear(); });
    }

    void SearchThread::StartThread() {
        {
            std::scoped_lock lock(mtx);
            active = true;
        }
        cond_var.notify_one();
    }

    bool SearchThread::MoveTimeLimitReached() const {
        return settings.limit_time && ElapsedSince(start_time) >= settings.allowed_time;
    }
    bool SearchThread::DepthLimitReached() const {
        return settings.limit_depth && depth_reached >= settings.allowed_depth;
    }
    bool SearchThread::NodesLimitReached() const {
        return settings.limit_nodes && NodesTotal() >= settings.allowed_nodes;
    }
    bool SearchThread::LimitReached() const {
        return !settings.infinite && (DepthLimitReached() || NodesLimitReached() || MoveTimeLimitReached());
    }
}
