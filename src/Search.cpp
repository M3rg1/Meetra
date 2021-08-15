#include "Search.h"
#include "MoveGen.h"
#include <chrono>
#include "Uci.h"
#include <random>
#include "SearchThread.h"
#include <sstream>

namespace Meetra::Search {

    std::vector<std::unique_ptr<SearchThread>> search_threads;

    void SetNumThreads(int num) {
        Globals::num_threads = std::clamp(num, 1, MAX_SEARCH_THREADS);
        for (auto &thread : search_threads) {
            thread->Shutdown();
        }
        search_threads.clear();
        for (int thread_id = 0; thread_id < Globals::num_threads; thread_id++) {
            search_threads.emplace_back(new SearchThread(thread_id));
        }
    }

    void Shutdown(){
        StopSearch();
        for (auto &thread : search_threads) {
            thread->Shutdown();
        }
    }

    void Init() {
        Globals::tt.Init();
        Globals::pvt.Init();
        Globals::run = false;
        Globals::show_currline = false;
        Globals::multi_pv = 1;
        Globals::show_currmove = true;
        Globals::plies_muted = 1;
        Globals::num_threads = DEFAULT_SEARCH_THREADS;
        Globals::plies_draw = DEFAULT_PLY_FOR_DRAW;
        for (int thread_id = 0; thread_id < Globals::num_threads; thread_id++) {
            search_threads.emplace_back(new SearchThread(thread_id));
        }
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

        // initialize search related global variables and calculate remaining time, generate root moves
        InitSearch(s, board);
        auto root_moves = GenRootMoves(board);

        // if there's only one root move and we are not in infinite or fixed depth/time search, return the only possible
        // move immediately
        if ((root_moves.size() == 1 && (!Globals::settings.infinite || !Globals::settings.fixed_timer)) ||
            root_moves.empty()) {
            StopSearch();
            Move only_move = root_moves.empty() ? INVALID_MOVE : root_moves[0].move;
            Uci::SendToGui("bestmove " + GetMoveName(only_move));
            return;
        }

        // activate timeout timer for search
        if (!Globals::settings.infinite) {
            Globals::search_timer.SetTimeout([&]() { StopSearch(); }, Globals::settings.allowed_time);
        }

        // activate timer that updates GUI with search info on interval
        Globals::info_timer.SetInterval([]() {
            Uci::SendToGui(GetUpdateSearchInfo());
        }, Globals::settings.info_to_ui_ms_timer);

        // prepare and activate helper search threads
        std::sort(root_moves.begin(), root_moves.end());
        for (int i = 1; i < search_threads.size(); i++) {
            search_threads[i]->InitNewSearch(board, root_moves);
            search_threads[i]->StartHelperThread();
        }

        // start the main search thread
        search_threads[0]->InitNewSearch(board, root_moves);
        search_threads[0]->Search();

        // await until all threads finish searching
        for (auto &st : search_threads) {
            while (!st->IsFinished());
        }

        // update each root move with the best search result
        for (auto &st : search_threads) {
            for (RootMove &m : st->GetRootMoves()) {
                for (RootMove &rm : root_moves) {
                    if (m.move == rm.move && m.depth > rm.depth) {
                        rm = m;
                    }
                }
            }
        }

        // select the best move
        RootMove best_move = root_moves[0];
        for (RootMove m : root_moves) {
            if (m.score > best_move.score) {
                best_move = m;
            }
        }

        // TODO need to figure out how to find out with which thread the best move is accosicated
        //  also try printing it if it's not the main thread (id = 0), so we can actually see if it ever happens
        Uci::SendToGui(search_threads[0]->GetSearchInfo());
        Uci::SendToGui("bestmove " + GetMoveName(best_move.move));

        Globals::info_timer.Stop();
        Globals::search_timer.Stop();
    }
}
