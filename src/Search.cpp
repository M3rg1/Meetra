#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include <random>
#include "Book.h"
#include <algorithm>

namespace Meetra::Search {

    long long int ElapsedTimeMs() {
        auto now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();
        auto elapsed_ms = now - start_time;
        return elapsed_ms + 1;
    }

    bool EnoughTimeLeft() {
        if (settings.infinite || settings.fixed_time || settings.fixed_depth ||
            settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    int TimeReduction(const Board &b) {
        if (settings.moves_to_go) {
            return settings.moves_to_go + 2;
        }
        int reduction = 45 + std::min(b.GetPhase(), 20) - b.FullMoveClock();
        reduction = std::max(reduction, 20);
        return reduction;
    }

    void InitSearchTimer(Board &board) {

        if (!settings.infinite && !settings.fixed_time && !settings.fixed_depth) {
            auto time_left = board.ColorToMove() == WHITE ? settings.wtime : settings.btime;
            int reduction = TimeReduction(board);
            settings.allowed_time = time_left / reduction;
            settings.allowed_time += board.ColorToMove() == WHITE ? settings.winc : settings.binc;
        }

        settings.allowed_time -= move_overhead;
    }

    void InitSearch(SearchSettings &s, Board &board) {

        finished = false;

        last_update_time = 0;
        start_time = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();

        settings = s;

        tt.NewSearch();
        mt_depth.store(0, std::memory_order_relaxed);

        settings.allowed_depth = std::min(s.allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);
    }

    std::vector<RootMove> GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> moves;
        Move move;
        while ((move = move_gen.GetBestMove<NORMAL>())) {
            if (board.IsMoveLegal(move)) {
                moves.emplace_back(move);
            }
        }
        return moves;
    }

    void FinishSearch() {

        StopSearch();

        // select the thread with the best move overall
        // for multipv, because helper threads might skip some depths and not have the full pv, we can't safely use
        // their results without risking their pv for other than the top move will be very old from low depth or even
        // missing entirely
        SearchThread *best_thread = threads[0].get();
        if (multi_pv == 1) {
            for (size_t i = 1; i < threads.size(); i++) {
                while (threads[i]->IsThreadSearching()); // wait until thread finishes searching
                if (threads[i]->DidBeatMove(best_thread->GetBestRootMove())) {
                    best_thread = threads[i].get();
                }
            }
        }

        Uci::Send(best_thread->GetSearchInfo());
        Uci::Send("bestmove " + best_thread->GetBestRmName());
        finished = true;
    }

    void StartSearch(SearchSettings s, Board board) {

        run = true;
        InitSearch(s, board);

        if (use_book && !settings.fixed_depth && !settings.infinite && !settings.fixed_time && !chess960 &&
            board.FullMoveClock() <= BOOK_DEPTH / 2) {
            auto moves = Book::Probe(board);
            if (!moves.empty()) {
                StopSearch();
                std::ranges::sample(
                        moves,
                        std::back_inserter(moves),
                        1,
                        std::mt19937{std::random_device{}()}
                );
                Uci::Send("bestmove " + board.MoveToName(moves.back()));
                Search::finished = true;
                return;
            }
        }

        auto root_moves = GenRootMoves(board);

        // if there's only one root move, and we are not in infinite or fixed depth/time search, return it immediately
        if (root_moves.empty() ||
            (root_moves.size() == 1 && !settings.infinite && !settings.fixed_depth && !settings.fixed_time)) {
            StopSearch();
            Uci::Send("bestmove " + board.MoveToName(root_moves.empty() ? ZERO_MOVE : root_moves.front().move));
            Search::finished = true;
            return;
        }

        // it's important to first initialize all threads and only then start them
        std::ranges::for_each(threads, [&](auto &t) { t->InitNewSearch(board, root_moves); });
        std::ranges::for_each(threads, [&](auto &t) { t->StartThread(); });
    }

    void Init() {
        tt.Init();
        run = false;
        finished = true;
        chess960 = false;
        use_book = false;
        show_currline = false;
        multi_pv = 1;
        show_currmove = true;
        plies_muted = 1;
        last_update_time = 0;
        move_overhead = DEFAULT_OVERHEAD;
        num_threads = DEFAULT_SEARCH_THREADS;
        SetNumThreads(DEFAULT_SEARCH_THREADS);
    }

    void Shutdown() {
        StopSearch();
        threads.clear();
    }

    void SetNumThreads(int num) {

        if (num > MAX_SEARCH_THREADS || num < 1) {
            num = std::clamp(num, 1, MAX_SEARCH_THREADS);
            Uci::SendInfo("Invalid threads count! Initializing to: " + std::to_string(num) + " threads");
        }

        Shutdown();

        num_threads = num;
        for (auto i = 0; i < num_threads; i++) {
            threads.emplace_back(new SearchThread());
        }
    }
}
