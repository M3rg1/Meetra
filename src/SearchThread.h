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
        struct PVMoveLine;
    }

    class SearchThread {

    public:

        SearchThread();
        ~SearchThread();
        void InitNewSearch(const Board &b, const std::vector<Search::RootMove> &moves);
        void Shutdown();
        void StartThread();
        void Search();

        [[nodiscard]] bool DidBeatMove(const Search::RootMove &other) const;
        [[nodiscard]] Search::RootMove GetBestRootMove() const;
        [[nodiscard]] std::string GetBestRmName() const;
        [[nodiscard]] inline bool IsThreadSearching() const { return active.load(std::memory_order_acquire); };
        [[nodiscard]] inline uint64_t NodesExplored() const { return nodes_explored.load(std::memory_order_relaxed); }
        [[nodiscard]] std::string GetSearchInfo() const;
        [[nodiscard]] inline int GetId() const { return id; }

    private:

        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply, Search::PVMoveLine &pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        void CheckTimers();
        [[nodiscard]] std::string GetCurrLineInfo() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        [[nodiscard]] bool MateFound() const;
        [[nodiscard]] bool MateInHorizon() const;
        [[nodiscard]] inline bool IsMainThread() const { return id == 0; }


        inline static int next_id = 0;
        int id;

        Board board;
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
        size_t curr_rm_num;
        Depth depth_reached;
        Depth seldepth_reached;
        std::atomic<uint64_t> nodes_explored;
        Depth max_qsearch_ply;

        std::atomic<bool> active;
        std::jthread thread;
        std::condition_variable_any cond_var;
        std::mutex mtx;
    };
}


#endif //MEETRA_SEARCHTHREAD_H
