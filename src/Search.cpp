#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include "Uci.h"
#include <random>
#include <sstream>

namespace Meetra::Search {

    long ElapsedTimeMs() {
        long now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - Globals::timer_start;
        return elapsed_ms + 1;
    }

    bool EnoughTimeLeft() {
        if (Globals::settings.infinite || Globals::settings.fixed_timer ||
            Globals::settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
    }

    bool TimeRunOut() {
        if (!Globals::settings.infinite && Globals::settings.allowed_time < ElapsedTimeMs()) {
            return true;
        }
        return false;
    }

    void RequestTime(long time_ms) {
        Globals::settings.allowed_time = ElapsedTimeMs() + time_ms;
    }

    void InitSearchTimer(Board &board) {

        if (Globals::settings.infinite || Globals::settings.fixed_timer) {
            return;
        }

        auto time_left = board.ColorToMove() == WHITE ? Globals::settings.white_time : Globals::settings.black_time;
        if (time_left) {
            int moves_made = std::min((board.MovesMadeCount() + 1), 10);
            double factor = 2.0 - static_cast<double>(moves_made) / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - static_cast<double>(moves_made);
            Globals::settings.allowed_time = static_cast<long>(factor * target);
        }
    }

    void InitSearch(SearchSettings &s, Board &board) {

        Globals::finished = false;

        Globals::timer_start = time_point_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()).time_since_epoch().count();

        Globals::settings = s;

        Globals::tt.NewSearch();
        Globals::nodes_explored.store(0, std::memory_order_relaxed);
        Globals::curr_max_depth.store(0, std::memory_order_relaxed);
        Globals::seldepth.store(0, std::memory_order_relaxed);

        Globals::settings.max_allowed_depth = std::min(s.max_allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);
    }

    std::vector<RootMove> GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        std::vector<RootMove> moves;
        Move move;
        while ((move = move_gen.GetBestMove<false>())) {
            if (!board.MakeMove(move)) {
                board.UnmakeMove(move);
                continue;
            }
            board.UnmakeMove(move);
            moves.emplace_back(move);
        }
        return moves;
    }

    std::string GetUpdateSearchInfo() {

        auto elapsed_ms = Search::ElapsedTimeMs();
        auto nps = static_cast<uint64_t>(
                ((static_cast<double>(Globals::nodes_explored.load(std::memory_order_relaxed)) /
                  static_cast<double>(elapsed_ms))) * 1000.0);

        std::ostringstream oss;

        oss << "info depth " << static_cast<int>(Globals::curr_max_depth.load(std::memory_order_relaxed))
            << " seldepth " << static_cast<int>(Globals::seldepth.load(std::memory_order_relaxed))
            << " nodes " << (Globals::nodes_explored.load(std::memory_order_relaxed))
            << " time " << elapsed_ms
            << " nps " << nps
            << " hashfull " << static_cast<int>(Globals::tt.Usage() * 1000.0);

        return oss.str();
    }

    void FinishSearch() {
        // select the thread with the best move overall
        // for multipv, because helper threads might skip some depths and not have the full pv, we can't safely use
        // their results without risking their pv for other than the top move will be very old from low depth or even
        // missing entirely
        SearchThread *best_thread = Globals::search_threads[0].get();
        if (Globals::multi_pv == 1) {
            for (auto i = 1; i < Globals::search_threads.size(); i++) {
                while (Globals::search_threads[i]->IsThreadSearching()); // wait thread finishes search
                if (Globals::search_threads[i]->GetBestRootMove().score > best_thread->GetBestRootMove().score &&
                    Globals::search_threads[i]->GetBestRootMove().depth >= best_thread->GetBestRootMove().depth) {
                    best_thread = Globals::search_threads[i].get();
                }
            }
        }

        Uci::SendToGui(best_thread->GetSearchInfo());
        Uci::SendToGui("bestmove " + GetMoveName(best_thread->GetBestRootMove().move));

        Globals::info_timer.SetState(Timer::INACTIVE);
        StopSearch();
        Globals::finished = true;
    }

    void StartSearch(SearchSettings s, Board board) {
        // initialize search related global variables and calculate remaining time, generate root moves
        Globals::run = true;
        InitSearch(s, board);
        auto root_moves = GenRootMoves(board);

        // if there's only one root move, and we are not in infinite or fixed depth/time search, return the only
        // possible move immediately
        if ((root_moves.size() == 1 && (!Globals::settings.infinite || !Globals::settings.fixed_timer)) ||
            root_moves.empty()) {
            StopSearch();
            Uci::SendToGui("bestmove " + GetMoveName(root_moves.empty() ? ZERO_MOVE : root_moves[0].move));
            return;
        }

/*        // activate timeout timer for search
        if (!Globals::settings.infinite) {
            Globals::search_timer.SetTimeout([&]() { StopSearch(); }, Globals::settings.allowed_time);
        }*/

        // activate timer that updates GUI with search info on interval
        Globals::info_timer.SetState(Timer::ACTIVE);

        // initialize and start each thread
        std::sort(root_moves.begin(), root_moves.end());
        for (auto &search_thread : Globals::search_threads) {
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
        Globals::plies_draw = DEFAULT_PLY_FOR_DRAW;
        Globals::num_threads = DEFAULT_SEARCH_THREADS;
        for (int thread_id = 0; thread_id < Globals::num_threads; thread_id++) {
            Globals::search_threads.emplace_back(new SearchThread(thread_id));
        }
    }

    void ShutdownThreads() {
        StopSearch();
        for (auto &t : Globals::search_threads) {
            t->Shutdown();
        }
        Globals::search_threads.clear();
    }

    void Shutdown() {
        StopSearch();
        ShutdownThreads();
        Globals::info_timer.ShutdownTimer();
    }

    void SetNumThreads(int num) {
        ShutdownThreads();
        Globals::num_threads = std::clamp(num, 1, MAX_SEARCH_THREADS);
        for (int thread_id = 0; thread_id < Globals::num_threads; thread_id++) {
            Globals::search_threads.emplace_back(new SearchThread(thread_id));
        }
    }

    Timer::Timer() {
        active = false;
        thread = std::jthread([&](const std::stop_token &stop_token) {
            while (true) {
                {
                    std::unique_lock lock(mtx);
                    if (active == INACTIVE) {
                        cond_var.wait(lock, stop_token, [&] { return active; });
                    } else {
                        cond_var.wait_for(lock, stop_token, std::chrono::milliseconds(DEFAULT_INFO_INTERVAL),
                                          [&] { return !active; });
                    }
                    if (stop_token.stop_requested()) { return; }
                    if (!active) continue;
                }
                Uci::SendToGui(GetUpdateSearchInfo());
                if (Search::Globals::show_currline) {
                    Uci::SendToGui(Globals::search_threads[0]->GetCurrLineInfo());
                }
            }
        });
    }

    Timer::~Timer() {
        ShutdownTimer();
    }

    void Timer::SetState(STATE status) {
        {
            std::scoped_lock lock(mtx);
            active = status;
        }
        cond_var.notify_one();
    }

    void Timer::ShutdownTimer() {
        if (thread.joinable()) {
            thread.request_stop();
            thread.join();
        }
    }
}
