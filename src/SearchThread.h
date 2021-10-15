#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Board.h"
#include "Config.h"

namespace Search {

    struct RootMove;
    struct PVLine;

    class SearchThread {

    public:

        explicit SearchThread(int id);
        void InitNewSearch(const Board &b, const std::vector<RootMove> &moves);
        void StartThread();
        void Search();
        void WaitForFinish();

        [[nodiscard]] bool DidBeatMove(const RootMove &other) const;
        [[nodiscard]] RootMove GetBestRootMove() const;
        [[nodiscard]] std::string GetBestRmName() const;
        [[nodiscard]] std::string GetSearchInfo() const;
        [[nodiscard]] inline uint64_t Nodes() const { return nodes_explored.load(std::memory_order_relaxed); }

    private:

        template<Node NodeType>
        Score ABSearch(Score alpha, Score beta, Depth depth, Depth ply, PVLine &pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        void CheckTimers();
        [[nodiscard]] std::string GetCurrLineInfo() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        [[nodiscard]] inline bool IsMainThread() const { return id == 0; }

        void InitThread(const std::stop_token &stop_token);

        Board board;
        Move killers[MAX_SEARCH_DEPTH + 2][2];
        std::vector<RootMove> root_moves;
        RootMove *curr_rm;
        size_t curr_rm_num;
        Depth depth_reached;
        Depth seldepth_reached;
        std::atomic<uint64_t> nodes_explored;

        int id;
        bool active;
        std::mutex mtx;
        std::condition_variable_any cond_var;
        std::jthread thread;
    };
}

#endif //MEETRA_SEARCHTHREAD_H
