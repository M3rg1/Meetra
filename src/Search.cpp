#include <sstream>
#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include "Uci.h"

namespace Meetra::Search {

    long ElapsedTimeMs() {
        long now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - Globals::start_time;
        return elapsed_ms + 1;
    }

    bool EnoughTimeLeft() {
        if (Globals::settings.infinite || Globals::settings.fixed_time || Globals::settings.fixed_depth ||
            Globals::settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    void RequestTime(long time_ms) {
        Globals::settings.allowed_time = ElapsedTimeMs() + time_ms;
    }

    void InitSearchTimer(Board &board) {

        if (Globals::settings.infinite || Globals::settings.fixed_time || Globals::settings.fixed_depth) {
            return;
        }

        auto time_left = board.ColorToMove() == WHITE ? Globals::settings.white_time : Globals::settings.black_time;
        if (time_left) {
            int moves_made = std::min((board.HistorySize() + 1), 10);
            double factor = 2.0 - static_cast<double>(moves_made) / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - static_cast<double>(moves_made);
            Globals::settings.allowed_time = static_cast<long>(factor * target);
            Globals::settings.allowed_time += board.ColorToMove() == WHITE ? Globals::settings.white_increment
                                                                           : Globals::settings.black_increment;
        }
    }

    void InitSearch(SearchSettings &s, Board &board) {

        Globals::finished = false;

        Globals::last_update_time = 0;
        Globals::start_time = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();

        Globals::settings = s;

        Globals::tt.NewSearch();
        Globals::mt_depth.store(0, std::memory_order_relaxed);

        Globals::settings.max_allowed_depth = std::min(s.max_allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);
    }

    std::vector<RootMove> GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> moves;
        Move move;
        while ((move = move_gen.GetBestMove<MoveGen::NORMAL>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            board.UnmakeMove(move);
            moves.emplace_back(move);
        }
        return moves;
    }

    void FinishSearch() {

        StopSearch();

        // select the thread with the best move overall
        // for multipv, because helper threads might skip some depths and not have the full pv, we can't safely use
        // their results without risking their pv for other than the top move will be very old from low depth or even
        // missing entirely
        SearchThread *best_thread = Globals::search_threads[0].get();
        if (Globals::multi_pv == 1) {
            for (auto i = 1; i < Globals::search_threads.size(); i++) {
                while (Globals::search_threads[i]->IsThreadSearching()); // wait thread finishes search
                if (Globals::search_threads[i]->DidBeatMove(best_thread->GetBestRootMove())) {
                    best_thread = Globals::search_threads[i].get();
                }
            }
        }

        Uci::SendToGui(best_thread->GetSearchInfo());
        Uci::SendToGui("bestmove " + GetMoveName(best_thread->GetBestRootMove().move));
        Globals::finished = true;
    }

    void StartSearch(SearchSettings s, Board board) {
        // initialize search related global variables and calculate remaining time, generate root moves
        Globals::run = true;
        InitSearch(s, board);
        auto root_moves = GenRootMoves(board);

        // if there's only one root move, and we are not in infinite or fixed depth/time search, return the only
        // possible move immediately
        if (root_moves.empty() ||
            (root_moves.size() == 1 &&
             (!Globals::settings.infinite || !Globals::settings.fixed_depth || !Globals::settings.fixed_time))) {
            StopSearch();
            Uci::SendToGui("bestmove " + GetMoveName(root_moves.empty() ? ZERO_MOVE : root_moves[0].move));
            return;
        }

        // initialize and start each thread
        std::sort(root_moves.begin(), root_moves.end());
        for (auto &search_thread: Globals::search_threads) {
            search_thread->InitNewSearch(board, root_moves);
            search_thread->StartThread();
        }
    }

    void Init() {
        Globals::tt.Init();
        Globals::run = false;
        Globals::finished = true;
        Globals::show_currline = false;
        Globals::multi_pv = 1;
        Globals::show_currmove = true;
        Globals::plies_muted = 1;
        Globals::num_threads = DEFAULT_SEARCH_THREADS;
        Globals::last_update_time = 0;
        SetNumThreads(DEFAULT_SEARCH_THREADS);
    }

    void ShutdownThreads() {
        StopSearch();
        for (auto &t: Globals::search_threads) {
            t->Shutdown();
        }
        Globals::search_threads.clear();
    }

    void Shutdown() {
        StopSearch();
        ShutdownThreads();
    }

    void SetNumThreads(int num) {

        if (num > MAX_SEARCH_THREADS || num < 1) {
            auto init_to = std::clamp(num, 1, MAX_SEARCH_THREADS);
            Uci::SendToGui("Invalid threads count! Initializing to: " + std::to_string(init_to) + " threads");
            SetNumThreads(init_to);
        }

        Globals::num_threads = num;
        ShutdownThreads();
        for (auto i = 0; i < Globals::num_threads; i++) {
            Globals::search_threads.emplace_back(new SearchThread());
        }
    }
}
