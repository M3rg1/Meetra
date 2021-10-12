#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include <algorithm>
#include "Board.h"
#include "Evaluator.h"
#include "TranspositionTable.h"
#include "SearchThread.h"
#include "Time.h"
#include "Defs.h"
#include "Config.h"

namespace Meetra::Search {

    struct SearchSettings {

        bool infinite = false;

        TimeRep allowed_time = INT64_MAX;
        uint64_t allowed_nodes = UINT64_MAX;
        Depth allowed_depth = MAX_SEARCH_DEPTH;

        int moves_to_go = 0;
        TimeRep wtime = DEFAULT_SEARCH_TIME;
        TimeRep btime = DEFAULT_SEARCH_TIME;
        TimeRep winc = 0;
        TimeRep binc = 0;
    };

    inline SearchSettings settings;
    inline std::atomic<bool> run;
    inline std::atomic<Depth> mt_depth;
    inline bool chess960;
    inline bool use_book;
    inline bool show_currline;
    inline bool show_currmove;
    inline int plies_muted;
    inline int multi_pv;
    inline TimeRep last_update_time;
    inline TimeRep move_overhead;
    inline Time::TimePoint start_time;
    inline TranspositionTable tt;
    inline std::vector<std::unique_ptr<SearchThread>> threads;

    struct PVMoveLine {
    private:
        Move moves[MAX_SEARCH_DEPTH + 1];
        size_t len = 0;
    public:
        [[nodiscard]] size_t Size() const { return len; }
        [[nodiscard]] Move At(size_t idx) const { return moves[idx]; }
        void PutMove(Move m) { moves[len++] = m; }
        void Clear() { len = 0; }
        void PutLine(const PVMoveLine &other) {
            std::copy_n(other.moves, other.len, moves + len);
            len += other.len;
        }
    };

    struct RootMove {
        Move move;
        PVMoveLine pv;
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

        bool operator!=(const RootMove &other) const {
            return !(*this == other);
        }
    };

    void Init();
    void StartSearch(SearchSettings settings, Board board);
    void FinishSearch();
    void Shutdown();

    [[nodiscard]] bool EnoughTimeLeft();
    [[nodiscard]] inline bool Run() { return run.load(std::memory_order_relaxed); }
    [[nodiscard]] inline Depth MtDepth() { return mt_depth.load(std::memory_order_relaxed); }
    inline void WaitFinished() { std::ranges::for_each(threads, [&](auto &t) { t->WaitForFinish(); }); }
    inline void StopSearch() { run = false; }
    inline void ClearTT() { tt.Clear(); }
    inline void ShowShowCurrLine(bool show) { show_currline = show; }
    inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
    inline void SetPliesMuted(int ply_muted) { plies_muted = std::max(0, ply_muted); }
    inline void SetMultiPv(int pv_num) { multi_pv = std::max(1, pv_num); }
    inline void SetTTSize(int size_mb) { tt.Init(size_mb); }
    inline void SetUseBook(bool use) { use_book = use; }
    inline void SetChess960(bool set) { chess960 = set; }
    inline void SetMoveOverhead(TimeRep overhead) { move_overhead = std::clamp(overhead, MIN_OVERHEAD, MAX_OVERHEAD); }
    void SetNumThreads(int num_threads);
    uint64_t NodesTotal();
}

#endif //MEETRA_SEARCH_H
