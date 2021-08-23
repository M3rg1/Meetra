#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include "Uci.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Board.h"

namespace Meetra {

    namespace Search {
        struct RootMove;
    }

    class SearchThread {

    public:

        SearchThread();
        ~SearchThread();
        void InitNewSearch(Board b, std::vector<Search::RootMove> moves);
        void Shutdown();
        void StartThread();

        [[nodiscard]] Search::RootMove GetBestRootMove() const;
        [[nodiscard]] inline bool IsThreadSearching() const { return active.load(std::memory_order_relaxed); };
        [[nodiscard]] inline ulong NodesExplored() const { return nodes_explored.load(std::memory_order_relaxed); }
        void CheckTimers();
        void Search();
        [[nodiscard]] std::string GetSearchInfo() const;

    private:

        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply, std::vector<Move> &pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        [[nodiscard]] std::string GetCurrLineInfo() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        [[nodiscard]] bool MateFound() const;
        [[nodiscard]] bool MateInHorizon() const;
        [[nodiscard]] inline bool IsMainThread() const { return id == 0; }


        inline static int threads_n = 0;
        int id;

        Board board;
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
        size_t curr_rm_num;
        Depth depth_reached;
        Depth seldepth_reached;
        std::atomic<uint64_t> nodes_explored;

        std::atomic<bool> active;
        std::jthread thread;
        std::condition_variable_any cond_var;
        std::mutex mtx;
    };
}


#endif //MEETRA_SEARCHTHREAD_H
