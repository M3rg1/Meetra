#include <random>
#include <syncstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include "Search.h"
#include "MoveGen.h"
#include "Book.h"

namespace Search {

    bool EnoughTimeLeft() {
        if (IsSearchLimited() || time_limit * 2 > ElapsedSince(start_time) * 3) {
            return true;
        }
        return false;
    }

    int TimeReduction(const Board &board) {
        int reduction = std::max(45 + std::min(board.Phase(), 20) - board.FullMoveClock(), 20);
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
                               [](auto sum, const auto &t) { return sum + t->Nodes(); });
    }

    std::vector<RootMove> GenRootMoves(const Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> root_moves;
        while (Move move = move_gen.NextMove()) {
            if (board.IsMoveLegal(move)) {
                root_moves.emplace_back(move);
                root_moves.back().score = board.MoveEval(move);
            }
        }
        std::ranges::sort(root_moves);
        return root_moves;
    }

    SearchThread *PickBestThread() {
        // select the thread with the best move overall
        // for multipv, because helper threads might skip some depths and not have the full pv, we can't safely use
        // their results without risking their pv for other than the top move will be very old from low depth or even
        // missing entirely
        auto best_thread = threads.front().get();
        if (multi_pv == 1) {
            for (auto &thread: threads | std::views::drop(1)) {
                thread->WaitForFinish();
                if (thread->DidBeatMove(best_thread->BestRootMove())) {
                    best_thread = thread.get();
                }
            }
        }
        return best_thread;
    }

    void FinalizeSearch() {
        StopSearch();
        auto best_thread = PickBestThread();
        best_thread->SendFullSearchInfo();
        best_thread->SendBestMove();
    }

    bool GetBookMove(const Board &board) {
        if (!IsSearchLimited() && !board.IsChess960() && board.FullMoveClock() <= BOOK_DEPTH / 2) {
            if (auto moves = Book::Probe(board.Hash()); !moves.empty()) {
                std::ranges::shuffle(moves, std::mt19937{std::random_device{}()});
                return moves.front();
            }
        }
        return ZERO_MOVE;
    }

    void StartSearch(Settings s, const Board &board) {

        Search::WaitFinished();

        run = true;
        InitNewSearch(s, board);

        if (use_book) {
            if (auto book_move = GetBookMove(board); book_move != ZERO_MOVE) {
                std::osyncstream(std::cout) << "bestmove " << board.MoveToStr(book_move) << std::endl;
                StopSearch();
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
        WaitFinished();
        threads.clear();
    }

    void SetNumThreads(int num_threads) {

        if (num_threads > MAX_SEARCH_THREADS || num_threads < MIN_SEARCH_THREADS) {
            num_threads = std::clamp(num_threads, MIN_SEARCH_THREADS, MAX_SEARCH_THREADS);
            std::osyncstream(std::cout)
                    << "info Invalid threads count! Initializing to: " << num_threads << " threads" << std::endl;
        }

        Shutdown();

        for (auto i: std::views::iota(0, num_threads)) {
            threads.emplace_back(new SearchThread(i));
        }
    }
}
