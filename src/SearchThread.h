#ifndef MEETRA_SEARCHTHREAD_H
#define MEETRA_SEARCHTHREAD_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Board.h"
#include "Config.h"
#include <algorithm>

namespace Search {

    struct PVLine {
        [[nodiscard]] auto begin() const { return Iterator<const Move>{moves}; }
        [[nodiscard]] auto end() const { return Iterator<const Move>{moves + Size()}; }
        [[nodiscard]] int Size() const { return len; }
        void PutMove(Move m) { moves[len++] = m; }
        void Clear() { len = 0; }
        void PutLine(const PVLine &other) {
            std::copy_n(other.moves, other.len, moves + len);
            len += other.len;
        }
    private:
        Move moves[MAX_SEARCH_DEPTH + 1];
        int len = 0;
    };

    struct RootMove {
        Move move;
        PVLine pv = {};
        Score score = NEGATIVE_INF;
        Score previous_score = NEGATIVE_INF;
        Depth depth = 0;
        Depth seldepth = 0;
        uint64_t nodes = 0;

        explicit RootMove(Move m) : move(m) {}

        std::strong_ordering operator<=>(const RootMove &other) const {
            return other.score != score ? other.score <=> score :
                   other.previous_score != previous_score ? other.previous_score <=> previous_score :
                   other.nodes <=> nodes;
        }

        bool operator==(const RootMove &other) const {
            return move == other.move;
        }
    };

    class SearchThread {

    public:

        explicit SearchThread(int id);
        void InitNewSearch(const Board &b, const std::vector<RootMove> &moves);
        void StartThread();
        void Search();
        void WaitForFinish();
        [[nodiscard]] bool LimitReached() const;

        [[nodiscard]] bool DidBeatMove(const RootMove &move) const;
        [[nodiscard]] RootMove GetBestRootMove() const;
        [[nodiscard]] inline uint64_t Nodes() const { return nodes_explored.load(std::memory_order_relaxed); }
        void SendBestMove() const;
        void SendFullSearchInfo() const;

    private:

        void InitThread(std::stop_token stop_token);

        template<Node NodeType>
        Score ABSearch(Score alpha, Score beta, Depth depth, Depth ply);
        Score QSearch(Score alpha, Score beta, Depth ply);
        void UpdateKillers(Move move, Depth ply);
        [[nodiscard]] Score RandomizedDrawScore() const;

        [[nodiscard]] bool MoveTimeLimitReached() const;
        [[nodiscard]] bool DepthLimitReached() const;
        [[nodiscard]] bool NodesLimitReached() const;

        void CheckTermination();
        void SendCurrLineInfo() const;
        void SendCurrMoveInfo() const;
        void SendBriefSearchInfo() const;
        [[nodiscard]] Depth GetMaxSeldepth() const;
        [[nodiscard]] inline bool IsMainThread() const { return id == 0; }

        Board board;
        std::array<std::array<Move, KILLER_SLOTS>, MAX_SEARCH_DEPTH + 1> killers;
        std::array<PVLine, MAX_SEARCH_DEPTH + 1> pv;
        std::vector<RootMove> root_moves;
        RootMove *curr_rm;
        int curr_rm_num;
        Depth depth_reached;
        std::atomic<uint64_t> nodes_explored;

        int id;
        bool active;
        std::mutex mtx;
        std::condition_variable_any cond_var;
        std::jthread thread;
    };
}

#endif //MEETRA_SEARCHTHREAD_H
