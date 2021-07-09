#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>

namespace Meetra {

    ABSearch::ABSearch() {
        run = false;
        tt = new TranspositionTable();
        InitSearch();
    }

    void ABSearch::InitSearch() {
        tt->NewSearch();
        pv_table.Reset();
        tt_hits = 0;
        mate_found = false;
        curr_move = INVALID_MOVE;
        curr_move_num = 0;
        best_move = INVALID_MOVE;
        best_score = NEGATIVE_INF;
        nodes_searched = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_depth = 0;
        mate_depth = 0;
        using namespace std::chrono;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    }

    void ABSearch::StartSearch(Board board, Depth max_depth, long allowed_time) {

        run = true;
        InitSearch();

        // TODO this timers shoulkdnt create new thread every time, but thread should already exist
        //  and we just send new tasks

        if (allowed_time != INFINITE_TIMER) {
            search_timer.SetTimeout([&]() { StopSearch(); }, allowed_time);
        }

        info_timer.SetInterval([&]() {
            std::cout << GetUpdateInfo() << std::endl;
            std::cout << GetCurrMoveInfo() << std::endl;
        }, 1000);

        for (curr_depth = 1; curr_depth <= max_depth; curr_depth++) {

            MoveGen move_gen(board);
            Score best_score_this_iter = NEGATIVE_INF;
            Move pv_move = INVALID_MOVE;
            curr_move_num = 0;

            while ((curr_move = move_gen.GetNextMove<false>())) {
                if (!board.MakeMove(curr_move)) {
                    board.UnmakeMove(curr_move);
                    continue;
                }
                curr_move_num++;
                nodes_searched++;
                Score score = -NegaMax(board, NEGATIVE_INF, POSITIVE_INF, curr_depth);
                board.UnmakeMove(curr_move);
                if (score > best_score_this_iter) {
                    best_score_this_iter = score;
                    pv_move = curr_move;
                }
            }

            if (run) {
                //pv_table->AddEntry(pv_move);
                best_move = pv_move;
                best_score = best_score_this_iter;
                if (std::abs(best_score) == MATE_SCORE) {
                    mate_found = true;
                    mate_depth = curr_depth;
                }
                std::cout << GetFullInfo() << std::endl;
            }

            if (!run || (allowed_time != INFINITE_TIMER && NotEnoughTimeLeft(allowed_time)) || !pv_move || mate_found) {
                break;
            }
        }
        run = false;
        search_timer.Stop();
        info_timer.Stop();
        std::cout << GetBestMove() << std::endl;
    }

    Score ABSearch::NegaMax(Board &board, Score alpha, Score beta, Depth depth) {

        if (board.Ply() >= 50  /*|| repetition*/ ) {
            return DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(board, alpha, beta, 1);
        }

        Score score = tt->GetEval(board.GetZobristHash(), alpha, beta, depth);
        if (score != NOT_FOUND) {
            tt_hits++;
            return score;
        }

        MoveGen move_gen(board, tt);
        Move move;
        Move best_move_this_iter = INVALID_MOVE;
        EntryFlag tt_flag = ALPHA;
        bool no_legal_moves = true;

        while ((move = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            nodes_searched++;
            score = -NegaMax(board, -beta, -alpha, depth - 1);
            board.UnmakeMove(move);
            if (!run) {
                return 0;
            } else if (score >= beta) {
                tt->AddEntry(board.GetZobristHash(), beta, depth, move, BETA);
                return beta;
            } else if (score > alpha) {
                tt_flag = EXACT_SCORE;
                alpha = score;
                best_move_this_iter = move;
            }
            no_legal_moves = false;
        }

        if (no_legal_moves) {
            if (move_gen.IsKingInCheck()) {
                return -MATE_SCORE;
            }
            return DRAW_SCORE;
        }

        // we could have a if lock check inside the TT, that will that will only allow write of new entries if run == true
        tt->AddEntry(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag);

        return alpha;
    }

    Score ABSearch::QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth) {

        if (depth > qsearch_depth) {
            qsearch_depth = depth;
        }

        auto score = BoardEval(board);
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }

        // TODO in qsearch try not to order moves by position, just by victim/attacker .. maybe?
        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetNextMove<true>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            qsearch_nodes++;
            score = -QuiescenceSearch(board, -beta, -alpha, depth + 1);
            board.UnmakeMove(move);
            if (score >= beta) {
                return beta;
            } else if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    bool ABSearch::NotEnoughTimeLeft(long allowed_time) const {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed = now - timer_start;
        if (allowed_time < elapsed * 2) {
            return true;
        }
        return false;
    }

    void ABSearch::ResizeTT(TTSize size) {
        tt->Resize(size);
    }

    void ABSearch::ClearTT() {
        tt->Clear();
    }

    std::string ABSearch::GetBestMove() const {
        std::string ret = "bestmove ";
        ret.append(GetMoveName(best_move));
        return ret;
    }

    std::string ABSearch::GetFullInfo() const {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        elapsed_ms = std::max(1l, elapsed_ms);
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        ss << "info depth " << curr_depth << " nodes " << (nodes_searched + qsearch_nodes) << " time "
           << elapsed_ms << " nps " << nps << " pv " << GetMoveName(best_move) << " hashfull "
           << static_cast<int>(tt->Usage() * 1000);
        if (mate_found) {
            ss << " score mate ";
            best_score == MATE_SCORE ? ss << (mate_depth + 1) / 2 : ss << (-(mate_depth + 1) / 2);
        } else {
            ss << " score cp " << best_score;
        }
        return ss.str();
    }

    std::string ABSearch::GetUpdateInfo() const {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        elapsed_ms = std::max(1l, elapsed_ms);
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        ss << "info nodes " << (nodes_searched + qsearch_nodes) << " time " << elapsed_ms << " nps " << nps
           << " hashfull " << static_cast<int>(tt->Usage() * 1000);

        return ss.str();
    }

    std::string ABSearch::GetCurrMoveInfo() const {
        std::string ret = "info currmove ";
        ret.append(GetMoveName(curr_move));
        ret.append(" currmovenumber ");
        ret.append(std::to_string(curr_move_num));
        return ret;
    }
}
