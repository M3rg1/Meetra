#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include "Uci.h"
#include <random>
#include "SearchThread.h"
#include "Timer.h"
#include "ThreadPool.h"
#include <sstream>

namespace Meetra::Search {

    void Init() {
        Globals::tt.Init();
        Globals::pvt.Init();
        Globals::run = false;
        Globals::show_currline = false;
        Globals::multi_pv = 1;
        Globals::show_currmove = true;
        Globals::plies_muted = 1;
        Globals::num_threads = DEFAULT_SEARCH_THREADS;
        // + prepare searchthreadsmu
    }

    long ElapsedTimeMs() {
        long now = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();
        long elapsed_ms = now - Globals::timer_start;
        return std::max(1l, elapsed_ms);
    }

    bool EnoughTimeLeft() {
        if (Globals::settings.infinite || Globals::settings.fixed_timer ||
            Globals::settings.allowed_time > ElapsedTimeMs() * 2) {
            return true;
        }
        return false;
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

        Globals::run = true;

        Globals::timer_start = time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count();

        Globals::settings = s;

        Globals::tt.NewSearch();
        Globals::pvt.NewSearch();
        Globals::nodes_explored = 0;
        Globals::curr_max_depth = 0;
        Globals::seldepth = 0;
        Globals::main_move = INVALID_MOVE;

        Globals::settings.max_allowed_depth = std::min(s.max_allowed_depth, static_cast<Depth>(MAX_SEARCH_DEPTH));

        InitSearchTimer(board);
    }

    std::vector<RootMove> GenRootMoves(Board &board) {
        MoveGen move_gen(board);
        Move move;
        std::vector<RootMove> moves;
        while ((move = move_gen.GetNextMove<false>())) {
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

        long elapsed_ms = Search::ElapsedTimeMs();
        long nps = static_cast<long>(static_cast<double>(Globals::nodes_explored) * 1000.0 /
                                     static_cast<double>(elapsed_ms));

        std::stringstream ss;

        ss << "info depth " << static_cast<int>(Globals::curr_max_depth)
           << " seldepth " << static_cast<int>(Globals::seldepth)
           << " nodes " << (Globals::nodes_explored)
           << " time " << elapsed_ms
           << " nps " << nps
           << " hashfull " << static_cast<int>(Globals::tt.Usage() * 1000.0);

        return ss.str();
    }

    void StartSearch(SearchSettings s, Board board) {

        InitSearch(s, board);
        auto root_moves = GenRootMoves(board);

        if (root_moves.size() < 2 && !Globals::settings.infinite) {
            StopSearch();
            Move only_move = root_moves.empty() ? INVALID_MOVE : root_moves[0].move;
            Uci::SendToGui("bestmove " + GetMoveName(only_move));
            return;
        }

        Timer search_timer;
        Timer info_timer;

        std::sort(root_moves.begin(), root_moves.end());

        for (auto t = Globals::num_threads - 1; t >= 0; t--) {
            SearchThread thread(t, board, root_moves);
            Globals::futures.push_back(ThreadPool::PushTask([=]() mutable {
                thread.Search();
            }));
        }

        // TODO rework how threadpool works, we should have
        //  1 dedicated thread to pooling information
        //  1 dedicated thread to stop timer
        //  1 dedicated thread to gui listening
        //  and then bunch of search threads
        if (!Globals::settings.infinite) {
            search_timer.SetTimeout([&]() { StopSearch(); }, Globals::settings.allowed_time);
        }

        info_timer.SetInterval([]() {
            Uci::SendToGui(GetUpdateSearchInfo());
        }, Globals::settings.info_to_ui_ms_timer);

        for (auto &future : Globals::futures) {
            future.wait();
        }
        Globals::futures.clear();

        Uci::SendToGui("bestmove " + GetMoveName(Globals::main_move));

        info_timer.Stop();
        search_timer.Stop();
    }
}
