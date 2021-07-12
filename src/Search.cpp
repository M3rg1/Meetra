#include "Search.h"
#include "MoveGen.h"
#include "Evaluation.h"
#include <iostream>
#include <chrono>
#include "TranspositionTable.h"
#include <sstream>
#include "ThreadPool.h"
#include "UciHandler.h"
#include <cstring>

namespace Meetra {

#define MULTI_PV 1

    ABSearch::ABSearch() {
        run = false;
        moves_count = 0;
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
        memset(move_evals, 0, MAX_LEGAL_MOVES);
        moves_count = 0;
        using namespace std::chrono;
        timer_start = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    }

    // TODO fixed search time isnt working right now! we ending when 50% time remaining!!!
    //  the UciHandler will have to let us know this is fixed search time, so we dont exist early via NotEnoughTime foo
    void ABSearch::StartSearch(const Board &b, Depth max_depth, long allowed_time) {

        board = b;
        run = true;
        InitSearch();

        // TODO this timers shoulkdnt create new thread every time, but thread should already exist
        //  and we just send new tasks

        if (allowed_time != INFINITE_TIMER) {
            search_timer.SetTimeout([&]() { StopSearch(); }, allowed_time);
        }
        info_timer.SetInterval([&]() {
            UciHandler::SendToGui(GetUpdateSearchInfo());
        }, 1000);

        MoveGen move_gen(board);
        Move m;
        while ((m = move_gen.GetNextMove<false>())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            Score s = tt.ProbeEval(board.GetZobristHash(), NEGATIVE_INF, POSITIVE_INF, 0, 0);
            board.UnmakeMove(m);
            move_evals[moves_count++].move = m;
            if (s != NOT_FOUND) {
                move_evals[moves_count].score = s;
            }
        }
        // if moves count == 0 = the board is already in checkmate/draw
        std::sort(move_evals, move_evals + moves_count,
                  [](const MoveAndEval &mae1, const MoveAndEval &mae2) {
                      return mae1.score > mae2.score;
                  });
        max_depth = std::min(max_depth, MAX_SEARCH_DEPTH);
        for (curr_max_depth = 1; curr_max_depth <= max_depth && run && EnoughTimeLeft(allowed_time); curr_max_depth++) {

            qsearch_depth = 0;
            Score alpha = NEGATIVE_INF;

            for (curr_move_num = 0; curr_move_num < moves_count; curr_move_num++) {
                curr_move = move_evals[curr_move_num].move;

                if (ElapsedTimeMs() > 1000) {
                    UciHandler::SendToGui(GetCurrMoveInfo());
                }

                board.MakeMove(curr_move);
                nodes_searched++;
                Score score = -NegaMax(alpha, POSITIVE_INF, curr_max_depth - 1, 1);
                board.UnmakeMove(curr_move);
                if (!run) {
                    break;
                }
                move_evals[curr_move_num].score = score;
                if (score > alpha) {
                    alpha = score;
                }
            }
            std::sort(move_evals, move_evals + moves_count,
                      [](const MoveAndEval &mae1, const MoveAndEval &mae2) {
                          return mae1.score > mae2.score;
                      });
            UciHandler::SendToGui(GetSearchInfo());

            if (IsScoreMate(move_evals[0].score)) {
                break;
            }
        }

        run = false;
        search_timer.Stop();
        info_timer.Stop();
        UciHandler::SendToGui(GetBestMove());
    }

    Score ABSearch::NegaMax(Score alpha, Score beta, Depth depth, Depth ply) {

        if (board.IsRepetition() || board.Ply() >= 50) {
            return DRAW_SCORE;
        } else if (depth == 0) {
            return QuiescenceSearch(alpha, beta, curr_max_depth);
        }

        Score score = tt.ProbeEval(board.GetZobristHash(), alpha, beta, depth, ply);
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
            score = -NegaMax(-beta, -alpha, depth - 1, ply + 1);
            board.UnmakeMove(move);
            if (!run) {
                return 0;
            } else if (score >= beta) {
                tt.SaveEval(board.GetZobristHash(), beta, depth, move, BETA, ply);
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
                return -MATE_SCORE + ply + 1;
            }
            return DRAW_SCORE;
        }

        tt.SaveEval(board.GetZobristHash(), alpha, depth, best_move_this_iter, tt_flag, ply);

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
        ret.append(GetMoveName(move_evals[0].move));
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
           << " hashfull " << static_cast<int>(tt.Usage() * 1000);

        return ss.str();
    }

    std::string ABSearch::GetSearchInfo() {

        long elapsed_ms = ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(nodes_searched + qsearch_nodes) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;
        Move pv_stack[MAX_SEARCH_DEPTH];
        auto pvs_to_send = std::min(MULTI_PV, moves_count);
        for (auto i = 0; i < pvs_to_send; i++) {
            ss << "info multipv " << i + 1
               << " depth " << +curr_max_depth
               << " seldepth " << qsearch_depth + curr_max_depth
               << " nodes " << (nodes_searched + qsearch_nodes)
               << " time " << elapsed_ms
               << " nps " << nps
               << " hashfull " << static_cast<int>(tt.Usage() * 1000)
               << " score ";

            int mate_length_ply = 0;
            if (move_evals[i].score >= MATE_SCORE - MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE - move_evals[i].score);
                ss << "mate " << mate_length_ply / 2;
            } else if (move_evals[i].score <= -MATE_SCORE + MAX_SEARCH_DEPTH) {
                mate_length_ply = static_cast<int>(MATE_SCORE + move_evals[i].score);
                ss << "mate " << -mate_length_ply / 2;
            } else {
                ss << "cp " << move_evals[i].score;
            }

            ss << " pv " << GetMoveName(move_evals[i].move);
            board.MakeMove(move_evals[i].move);
            // TODO try to remove the max depth guard when we have working repetition recognition
            RetrievePv(pv_stack, std::max(curr_max_depth - 1, mate_length_ply));
            board.UnmakeMove(move_evals[i].move);
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
