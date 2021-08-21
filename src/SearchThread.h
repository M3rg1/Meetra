#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include <iostream>
#include "Uci.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Board.h"
#include "Types.h"

namespace Meetra {

    namespace Search {
        struct RootMove;
    }

    class SearchThread {

    public:

        explicit SearchThread(int t_num) : thread_num(t_num) {
            active = false;
            thread = std::jthread([&](const std::stop_token &stop_token) {
                while (true) {
                    {
                        std::unique_lock lock(mtx);
                        cond_var.wait(lock, stop_token, [&] { return IsThreadSearching(); });
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
                {
                    std::scoped_lock lock(mtx);
                    active = false;
                }
                thread.request_stop();
                thread.join();
            }
        }

        void StartThread() {
            {
                std::scoped_lock lock(mtx);
                active = true;
            }
            cond_var.notify_one();
        };

        [[nodiscard]] std::string GetSearchInfo();
        [[nodiscard]] std::string GetCurrLineInfo();
        [[nodiscard]] Search::RootMove GetBestRootMove() const;
        [[nodiscard]] inline bool IsThreadSearching() const { return active.load(std::memory_order_relaxed); };
        void Search();

    private:

        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply, std::vector<Move> &pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        void BackupPv(std::vector<Move> &pv_line, Board &b, size_t max_len_pv);
        [[nodiscard]] std::string GetCurrMoveInfo();
        [[nodiscard]] bool MateFound() const;
        [[nodiscard]] bool MateInHorizon() const;
        [[nodiscard]] inline bool IsMainThread() const { return thread_num == 0; }

        int thread_num;
        Board board;
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
        int curr_rm_num;
        Depth curr_depth;

        std::atomic<bool> active;
        std::jthread thread;
        std::condition_variable_any cond_var;
        std::mutex mtx;
    };
}


#endif //MEETRA_SEARCHTHREAD_H
