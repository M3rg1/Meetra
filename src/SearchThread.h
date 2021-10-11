#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Board.h"
#include "Config.h"

namespace Meetra {

    namespace Search {
        struct RootMove;
        struct PVMoveLine;
    }

    class SearchThread {

    public:

        explicit SearchThread(int id);
        ~SearchThread();
        void InitNewSearch(const Board &b, const std::vector<Search::RootMove> &moves);
        void StartThread();
        void Search();
        void WaitForFinish();

        [[nodiscard]] bool DidBeatMove(const Search::RootMove &other) const;
        [[nodiscard]] Search::RootMove GetBestRootMove() const;
        [[nodiscard]] std::string GetBestRmName() const;
        [[nodiscard]] std::string GetSearchInfo() const;
        [[nodiscard]] inline uint64_t Nodes() const { return nodes_explored.load(std::memory_order_relaxed); }

    private:

        enum Node {
            PV, NONPV, NULLMOVE
        };

        template<Node NodeType>
        Score ABSearch(Score alpha, Score beta, Depth depth, Depth ply, Search::PVMoveLine &pv_line);
        Score QSearch(Score alpha, Score beta, Depth ply);

        void CheckTimers();
        [[nodiscard]] std::string GetCurrLineInfo() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        [[nodiscard]] inline bool IsMainThread() const { return id == 0; }

        void InitThread(const std::stop_token &stop_token);

        Board board;
        Move killers[MAX_SEARCH_DEPTH + 2][2];
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
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
