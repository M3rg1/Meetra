#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "ThreadPool.h"
#include "UciHandler.h"

namespace Meetra {

#define MULTI_PV 1

    ABSearch::ABSearch() {
        run = false;
        InitSearch();
    }

    void ABSearch::InitSearch() {
        tt.NewSearch();
        tt_hits = 0;
        curr_move = INVALID_MOVE;
        curr_move_num = 0;
        nodes_searched = 0;
        qsearch_nodes = 0;
        qsearch_depth = 0;
        curr_max_depth = 0;
        using namespace std::chrono;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    }

    // board should be class variable, so we dont have to pass it in the recursion all the time
    // TODO fixed search time isnt working right now! we ending when 50% time remaining!!!
    //  the UciHandler will have to let us know this is fixed search time, so we dont exist early via NotEnoughTime foo
    void ABSearch::StartSearch(const Board &b, Depth max_depth, long allowed_time) {

        board = b;
        run = true;
        InitSearch();
        std::string uci_send_info;

        // TODO this timers shoulkdnt create new thread every time, but thread should already exist
        //  and we just send new tasks

        if (allowed_time != INFINITE_TIMER) {
            search_timer.SetTimeout([&]() { StopSearch(); }, allowed_time);
        }
        info_timer.SetInterval([&]() {
            UciHandler::SendToGui(GetUpdateSearchInfo());
        }, 1000);

        MoveGen move_gen(board);
        moves_count = 0;
        Move m;
        while ((m = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            board.UnmakeMove(m);
            score_move_pair[moves_count++].second = m;
        }
        // if moves count == 0 = the board is already in checkmate/draw

        for (curr_max_depth = 1; curr_max_depth <= max_depth && run && EnoughTimeLeft(allowed_time); curr_max_depth++) {

            qsearch_depth = 0;

            for (curr_move_num = 0; curr_move_num < moves_count && run; curr_move_num++) {
                curr_move = score_move_pair[curr_move_num].second;

                if (ElapsedTimeMs() > 1000) {
                    uci_send_info = GetCurrMoveInfo();
                    ThreadPool::PushTask([uci_send_info]() { UciHandler::SendToGui(uci_send_info); });
                }

                board.MakeMove(curr_move);
                nodes_searched++;
                Score score = -NegaMax(NEGATIVE_INF, POSITIVE_INF, curr_max_depth - 1);
                if (run) {
                    score_move_pair[curr_move_num].first = score;
                }
                board.UnmakeMove(curr_move);
            }

            std::sort(score_move_pair, score_move_pair + moves_count, std::greater<>());
            uci_send_info = GetSearchInfo();
            ThreadPool::PushTask([uci_send_info]() { UciHandler::SendToGui(uci_send_info); });
        }

        run = false;
        search_timer.Stop();
        info_timer.Stop();
        uci_send_info = GetBestMove();
        // We dont really want to send any more info after sending bestmove, this almost guarantees that wont happen
        // (in case the final search info wasn't sent yet by the ThreadPool)
        ThreadPool::PushTask([uci_send_info]() { UciHandler::SendToGui(uci_send_info); });
    }

    Score ABSearch::NegaMax(Score alpha, Score beta, Depth depth) {

        if (board.Ply() >= 50  /*|| repetition*/ ) {
            return DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(alpha, beta, curr_max_depth);
        }

        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth);
        if (score != NOT_FOUND) {
            tt_hits++;
            return score;
        }

        MoveGen move_gen(board, &tt);
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
            score = -NegaMax(-beta, -alpha, depth - 1);
            board.UnmakeMove(move);
            if (!run) {
                return 0;
            } else if (score >= beta) {
                tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA);
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

        tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag);

        return alpha;
    }

    Score ABSearch::QuiescenceSearch(Score alpha, Score beta, Depth depth) {

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
            score = -QuiescenceSearch(-beta, -alpha, depth + 1);
            board.UnmakeMove(move);
            if (score >= beta) {
                return beta;
            } else if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    bool ABSearch::EnoughTimeLeft(long allowed_time) const {
        if (allowed_time == INFINITE_TIMER || allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    void ABSearch::ResizeTT(TTSize size) {
        tt.Resize(size);
    }

    void ABSearch::ClearTT() {
        tt.Clear();
    }

    std::string ABSearch::GetBestMove() const {
        std::string ret = "bestmove ";
        ret.append(GetMoveName(score_move_pair[0].second));
        return ret;
    }

    long ABSearch::ElapsedTimeMs() const {
        using namespace std::chrono;
        long now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - timer_start;
        return std::max(1l, elapsed_ms);
    }

    void ABSearch::RetrievePv(Move *pv_line, Depth depth) {
        Move move = tt.GetPVMove(board.GetZobristHash());
        if (!move || depth == 0) {
            *pv_line = INVALID_MOVE;
            return;
        }
        *pv_line++ = move;
        board.MakeMove(move);
        RetrievePv(pv_line, depth - 1);
        board.UnmakeMove(move);
    }

    std::string ABSearch::GetUpdateSearchInfo() const {

        auto elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << +curr_max_depth
           << " seldepth " << qsearch_depth + curr_max_depth
           << " nodes " << (nodes_searched + qsearch_nodes)
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull ";

        return ss.str();
    }

    std::string ABSearch::GetSearchInfo() {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[64];
        auto pvs_to_send = std::min(MULTI_PV, moves_count);
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info multipv " << i + 1
               << " depth " << +curr_max_depth
               << " seldepth " << qsearch_depth + curr_max_depth
               << " nodes " << (nodes_searched + qsearch_nodes)
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score cp " << score_move_pair[i].first
               << " pv " << GetMoveName(score_move_pair[i].second);

            board.MakeMove(score_move_pair[i].second);
            // TODO try to remove the max depth guard when we have working repetition recognition
            RetrievePv(pv_stack, curr_max_depth - 1);
            board.UnmakeMove(score_move_pair[i].second);
            Move *pv_stack_ptr = pv_stack;
            Move pv_move;
            while ((pv_move = *pv_stack_ptr++)) {
                ss << " " << GetMoveName(pv_move);
            }
        }

        return ss.str();
    }

    std::string ABSearch::GetCurrMoveInfo() const {
        std::string ret = "info currmove ";
        ret.append(GetMoveName(curr_move));
        ret.append(" currmovenumber ");
        ret.append(std::to_string(curr_move_num + 1));
        return ret;
    }
}
