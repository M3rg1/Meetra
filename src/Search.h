#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "Evaluation.h"
#include "TranspositionTable.h"
#include "SearchThread.h"

namespace Meetra::Search {

#define MAX_SEARCH_DEPTH 128
#define DEFAULT_SEARCH_TIME 1000
#define DEFAULT_SEARCH_THREADS 1
#define UPDATE_INFO_INTERVAL 1000
#define MAX_SEARCH_THREADS 8
#define MIN_MATE_EVAL (MATE_SCORE - MAX_SEARCH_DEPTH)

    struct SearchSettings {
        bool infinite = false;
        bool fixed_time = false;
        bool fixed_depth = false;

        long allowed_time = DEFAULT_SEARCH_TIME;
        Depth max_allowed_depth = MAX_SEARCH_DEPTH;

        long white_time = 0;
        long black_time = 0;
        long white_increment = 0;
        long black_increment = 0;
    };

    namespace Globals {
        inline std::atomic<bool> run;
        inline std::atomic<bool> finished;
        inline std::atomic<Depth> mt_depth;
        inline TranspositionTable tt;
        inline SearchSettings settings;
        inline bool show_currline;
        inline bool show_currmove;
        inline int plies_muted;
        inline int multi_pv;
        inline int num_threads;
        inline long start_time;
        inline long last_update_time;
        inline std::vector<std::unique_ptr<SearchThread>> search_threads;
    }

    struct PVMoveLine {
    private:
        Move moves[MAX_SEARCH_DEPTH];
        size_t len = 0;
    public:
        [[nodiscard]] size_t Size() const { return len; }
        [[nodiscard]] Move At(size_t idx) const { return moves[idx]; }
        void PutMove(Move m) { moves[len++] = m; }
        void Clear() { len = 0; }
        void PutLine(const PVMoveLine &line) {
            std::copy_n(std::begin(line.moves), line.len, std::begin(moves) + len);
            len += line.len;
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

        bool operator<(const RootMove &mn) const {
            return mn.score != score ? mn.score < score :
                   mn.previous_score != previous_score ? mn.previous_score < previous_score :
                   mn.nodes < nodes;
        }

        bool operator==(const RootMove &other) const {
            return move == other.move;
        }
    };

    void Init();
    void StartSearch(SearchSettings settings, Board board);
    void FinishSearch();
    void Shutdown();
    std::string GetUpdateSearchInfo();

    void RequestTime(long time_ms);
    [[nodiscard]] long ElapsedTimeMs();
    [[nodiscard]] bool EnoughTimeLeft();
    [[nodiscard]] inline bool Run() { return Globals::run.load(std::memory_order_relaxed); }
    [[nodiscard]] inline bool Finished() { return Globals::finished.load(std::memory_order_relaxed); }
    inline void ShowShowCurrLine(bool show) { Globals::show_currline = show; }
    inline void SetPliesMuted(int ply_muted) { Globals::plies_muted = std::max(1, ply_muted); }
    inline void ShowCurrMoveInfo(bool show) { Globals::show_currmove = show; }
    inline void StopSearch() { Globals::run = false; }
    inline void SetMultiPv(int pv_num) { Globals::multi_pv = std::max(1, pv_num); }
    inline void ClearTT() { Globals::tt.Clear(); }
    inline void SetTTSize(int size_mb) { Globals::tt.Resize(size_mb); }
    void SetNumThreads(int num);

}

#endif //MEETRA_SEARCH_H
