#include "Search.h"
#include "MoveGen.h"
#include <random>
#include <syncstream>
#include <iostream>
#include <ranges>
#include "Book.h"

namespace Search {

    bool EnoughTimeLeft() {
        if (IsSearchLimited() || time_limit * 3 > ElapsedSince(start_time) * 2) {
            return true;
        }
        return false;
    }

    int TimeReduction(const Board &b) {
        int reduction = std::max(45 + std::min(b.GetPhase(), 20) - b.FullMoveClock(), 20);
        if (settings.moves_to_go) {
            reduction = std::min(settings.moves_to_go + 1, reduction - 3);
        }
        return reduction;
    }

    void InitSearchTimer(const Board &board) {
        if (!IsSearchLimited()) {
            auto time_left = board.ColorToMove() == WHITE ? settings.wtime : settings.btime;
            time_limit = time_left / TimeReduction(board);
            time_limit += board.ColorToMove() == WHITE ? settings.winc : settings.binc;
            time_limit -= move_overhead;
        }
    }

    void InitNewSearch(const Settings &s, const Board &board) {
        start_time = Now();
        last_update_time = 0;
        mt_depth = 0;
        settings = s;
        tt.NewSearch();
        InitSearchTimer(board);
    }

    bool IsSearchLimited() {
        return settings.infinite || settings.limit_nodes || settings.limit_depth || settings.limit_time;
    }

    uint64_t NodesTotal() {
        return std::accumulate(threads.begin(),
                               threads.end(),
                               uint64_t{},
                               [&](auto sum, const auto &t) { return sum + t->Nodes(); });
    }

    std::vector<RootMove> GenRootMoves(const Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> root_moves;
        while (Move move = move_gen.GetBestMove<NORMAL>()) {
            if (board.IsMoveLegal(move)) {
                root_moves.emplace_back(move);
            }
        }
        for (auto &rm: root_moves) {
            rm.score = board.GetMoveEval(rm.move);
        }
        std::ranges::sort(root_moves);
        return root_moves;
    }

    void FinishSearch() {

        StopSearch();

        // select the thread with the best move overall
        // for multipv, because helper threads might skip some depths and not have the full pv, we can't safely use
        // their results without risking their pv for other than the top move will be very old from low depth or even
        // missing entirely
        SearchThread *best_thread = threads.front().get();
        if (multi_pv == 1) {
            for (size_t i = 1; i < threads.size(); ++i) {
                threads[i]->WaitForFinish();
                if (threads[i]->DidBeatMove(best_thread->GetBestRootMove())) {
                    best_thread = threads[i].get();
                }
            }
        }

        best_thread->SendFullSearchInfo();
        best_thread->SendBestMove();
    }

    void StartSearch(Settings s, Board board) {

        Search::WaitFinished();

        if (1) {
            auto best_score = 0;
            auto root_moves = GenRootMoves(board);
            Move best_move = root_moves.front().move;
            for (auto move : root_moves) {
                board.MakeMove(move.move);
                auto score = board.Eval();
                board.UnmakeMove(move.move);
                if (score > best_score) {
                    best_move = move.move;
                    best_score = score;
                }
            }
            std::osyncstream(std::cout) << "info score " << best_score << std::endl;
            std::osyncstream(std::cout) << "bestmove " << board.MoveToName(best_move) << std::endl;
            return;
        }

        run = true;
        InitNewSearch(s, board);

        if (use_book && !IsSearchLimited() && !board.IsChess960() && board.FullMoveClock() <= BOOK_DEPTH / 2) {
            if (auto moves = Book::Probe(board.GetHash()); !moves.empty()) {
                StopSearch();
                std::ranges::shuffle(moves, std::mt19937{std::random_device{}()});
                std::osyncstream(std::cout) << "bestmove " << board.MoveToName(moves.front()) << std::endl;
                return;
            }
        }

        auto root_moves = GenRootMoves(board);

        if (root_moves.empty()) {
            StopSearch();
            root_moves.emplace_back(ZERO_MOVE);
            if (board.IsInCheck()) root_moves.front().score = MATE_SCORE;
        }

        if (root_moves.size() == 1 && !IsSearchLimited()) {
            time_limit = std::min(time_limit, static_cast<TimeRep>(1000));
        }

        // it's important to first initialize all threads and only then start them
        std::ranges::for_each(threads, [&](auto &t) { t->InitNewSearch(board, root_moves); });
        if (threads.front()->LimitReached()) StopSearch(); // 0 nodes or 0 movetime or 0 depth search - stop immediately
        std::ranges::for_each(threads, [&](auto &t) { t->StartThread(); });
    }

    void Init() {
        SetNumThreads(DEFAULT_SEARCH_THREADS);
        tt.Init();
    }

    void Shutdown() {
        StopSearch();
        threads.clear();
    }

    void SetNumThreads(int num_threads) {

        if (num_threads > MAX_SEARCH_THREADS || num_threads < 1) {
            num_threads = std::clamp(num_threads, MIN_SEARCH_THREADS, MAX_SEARCH_THREADS);
            std::osyncstream(std::cout)
                    << "info Invalid threads count! Initializing to: " << num_threads << " threads" << std::endl;
        }

        Shutdown();

        for (auto i = 0; i < num_threads; ++i) {
            threads.emplace_back(new SearchThread(i));
        }
    }
}
