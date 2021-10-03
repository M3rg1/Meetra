#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "Evaluator.h"
#include "TranspositionTable.h"
#include "SearchThread.h"

namespace Meetra::Search {

#define MAX_SEARCH_DEPTH 128
#define DEFAULT_SEARCH_TIME static_cast<long long>(60000)
#define DEFAULT_SEARCH_THREADS 1
#define UPDATE_INFO_INTERVAL static_cast<long long>(1000)
#define MAX_SEARCH_THREADS 32
#define MIN_MATE_EVAL (MATE_SCORE - MAX_SEARCH_DEPTH)
#define MIN_OVERHEAD static_cast<long long>(0)
#define MAX_OVERHEAD static_cast<long long>(1000)
#define DEFAULT_OVERHEAD static_cast<long long>(5)

    struct SearchSettings {
        bool infinite = false;

        long long allowed_time = LONG_LONG_MAX;
        Depth allowed_depth = MAX_SEARCH_DEPTH;
        uint64_t allowed_nodes = UINT64_MAX;

        int moves_to_go = 0;
        long long wtime = DEFAULT_SEARCH_TIME;
        long long btime = DEFAULT_SEARCH_TIME;
        long long winc = 0;
        long long binc = 0;
    };

    inline SearchSettings settings;
    inline std::atomic<bool> run;
    inline std::atomic<bool> finished;
    inline std::atomic<Depth> mt_depth;
    inline bool chess960;
    inline bool use_book;
    inline bool show_currline;
    inline bool show_currmove;
    inline int plies_muted;
    inline int multi_pv;
    inline long long move_overhead;
    inline long long start_time;
    inline long long last_update_time;
    inline TranspositionTable tt;
    inline std::vector<std::unique_ptr<SearchThread>> threads;

    struct PVMoveLine {
    private:
        Move moves[MAX_SEARCH_DEPTH];
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

    [[nodiscard]] long long int ElapsedTimeMs();
    [[nodiscard]] bool EnoughTimeLeft();
    [[nodiscard]] inline bool Run() { return run.load(std::memory_order_relaxed); }
    [[nodiscard]] inline bool Finished() { return finished.load(std::memory_order_relaxed); }
    [[nodiscard]] inline Depth MtDepth() { return mt_depth.load(std::memory_order_relaxed); }
    inline void SetUseBook(bool use) { use_book = use; }
    inline void ShowShowCurrLine(bool show) { show_currline = show; }
    inline void SetPliesMuted(int ply_muted) { plies_muted = std::max(1, ply_muted); }
    inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
    inline void StopSearch() { run = false; }
    inline void SetMultiPv(int pv_num) { multi_pv = std::max(1, pv_num); }
    inline void ClearTT() { tt.Clear(); }
    inline void SetTTSize(int size_mb) { tt.Init(size_mb); }
    inline void SetChess960(bool set) { chess960 = set; }
    inline void SetMoveOverhead(long long overhead) { move_overhead = std::clamp(overhead, MIN_OVERHEAD, MAX_OVERHEAD); }
    void SetNumThreads(int num_threads);
    uint64_t NodesTotal();

}

#endif //MEETRA_SEARCH_H
