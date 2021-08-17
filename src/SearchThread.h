#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include "Search.h"
#include "Uci.h"

namespace Meetra {

    class SearchThread {

    public:

        explicit SearchThread(int t_num) : thread_num(t_num), searching(false) {
            thread = std::jthread([&](const std::stop_token &stop_token) {
                while (true) {
                    {
                        std::unique_lock lock(mtx);
                        cond_var.wait(lock, stop_token, [&] { return searching; });
                    }
                    if (stop_token.stop_requested()) { return; }
                    Search();
                }
            });
        }

        ~SearchThread() {
            Shutdown();
        }

        void InitNewSearch(Board b, std::vector<Search::RootMove> moves) {
            board = b;
            root_moves = std::move(moves);
            curr_rm = &root_moves[0];
            curr_rm_num = 0;
            curr_depth = 0;
        }

        void Shutdown() {
            if (thread.joinable()) {
                thread.request_stop();
                thread.join();
            }
        }

        void StartHelperThread() {
            {
                std::scoped_lock lock(mtx);
                searching = true;
            }
            cond_var.notify_one();
        };

        [[nodiscard]] std::string GetSearchInfo();
        [[nodiscard]] std::vector<Search::RootMove> GetRootMoves() { return root_moves; }
        [[nodiscard]] bool IsFinished() const { return !searching; };
        [[nodiscard]] Search::RootMove GetBestRootMove() const { return root_moves[0]; };
        void Search();

    private:

        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply, std::vector<Move>& pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        [[nodiscard]] std::string GetCurrMoveInfo();
        [[nodiscard]] bool MateInHorizon() const;
        [[nodiscard]] inline bool IsMainThread() const { return thread_num == 0; }

        int thread_num;
        Board board;
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
        int curr_rm_num;
        Depth curr_depth;

        bool searching;
        std::jthread thread;
        std::condition_variable_any cond_var;
        std::mutex mtx;
    };

}


#endif //MEETRA_SEARCHTHREAD_H
