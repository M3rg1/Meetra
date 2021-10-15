#include "Search.h"
#include "MoveGen.h"
#include <random>
#include "Book.h"
#include <algorithm>

namespace Meetra::Search {

    bool EnoughTimeLeft() {
        if (settings.infinite || settings.allowed_time > Time::ElapsedTime<Time::ms>(start_time) * 2) {
            return true;
        }
        return false;
    }

    int TimeReduction(const Board &b) {
        if (settings.moves_to_go) {
            return settings.moves_to_go + 2;
        }
        int reduction = 45 + std::min(b.GetPhase(), 20) - b.FullMoveClock();
        return std::max(reduction, 20);
    }

    uint64_t NodesTotal() {
        return std::accumulate(threads.begin(),
                               threads.end(),
                               0ULL,
                               [&](auto sum, auto const &t) { return sum + t->Nodes(); });
    }

    void InitSearchTimer(const Board &board) {

        if (!settings.infinite) {
            auto time_left = board.ColorToMove() == WHITE ? settings.wtime : settings.btime;
            auto reduction = TimeReduction(board);
            settings.allowed_time = time_left / reduction;
            settings.allowed_time += board.ColorToMove() == WHITE ? settings.winc : settings.binc;
        }

        settings.allowed_time -= move_overhead;
    }

    void InitSearch(const SearchSettings &s, const Board &board) {

        start_time = Time::Now();
        last_update_time = 0;

        settings = s;

        tt.NewSearch();
        mt_depth.store(0, std::memory_order_relaxed);

        settings.allowed_depth = std::min(s.allowed_depth, MAX_SEARCH_DEPTH);

        InitSearchTimer(board);
    }

    std::vector<RootMove> GenRootMoves(const Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> moves;
        Move move;
        while ((move = move_gen.GetBestMove<MoveGen::NORMAL>())) {
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
            for (size_t i = 1; i < threads.size(); ++i) {
                threads[i]->WaitForFinish();
                if (threads[i]->DidBeatMove(best_thread->GetBestRootMove())) {
                    best_thread = threads[i].get();
                }
            }
        }

        Uci::Send(best_thread->GetSearchInfo());
        Uci::Send("bestmove " + best_thread->GetBestRmName());
    }

    void StartSearch(SearchSettings s, Board board) {

        Search::WaitFinished();

        run = true;
        InitSearch(s, board);

        if (use_book && !settings.infinite && !chess960 && board.FullMoveClock() <= BOOK_DEPTH / 2) {
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
                return;
            }
        }

        auto root_moves = GenRootMoves(board);

        // if there's only one root move, and we are not in infinite or fixed depth/time search, return it immediately
        if (root_moves.empty() || (root_moves.size() == 1 && !settings.infinite)) {
            StopSearch();
            Uci::Send("bestmove " + board.MoveToName(root_moves.empty() ? ZERO_MOVE : root_moves.front().move));
            return;
        }

        // it's important to first initialize all threads and only then start them
        std::ranges::for_each(threads, [&](auto &t) { t->InitNewSearch(board, root_moves); });
        std::ranges::for_each(threads, [&](auto &t) { t->StartThread(); });
    }

    void Init() {
        tt.Init();
        run = false;
        chess960 = false;
        use_book = false;
        show_currline = false;
        multi_pv = 1;
        show_currmove = true;
        plies_muted = 0;
        last_update_time = 0;
        move_overhead = DEFAULT_OVERHEAD;
        SetNumThreads(DEFAULT_SEARCH_THREADS);
    }

    void Shutdown() {
        StopSearch();
        threads.clear();
    }

    void SetNumThreads(int num_threads) {

        if (num_threads > MAX_SEARCH_THREADS || num_threads < 1) {
            num_threads = std::clamp(num_threads, 1, MAX_SEARCH_THREADS);
            Uci::SendInfo("Invalid threads count! Initializing to: " + std::to_string(num_threads) + " threads");
        }

        Shutdown();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(new SearchThread(i));
        }
    }
}
