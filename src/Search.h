#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "Evaluator.h"
#include "TranspositionTable.h"
#include "SearchThread.h"
#include "Defs.h"
#include "Config.h"

#include <algorithm>

namespace Search {

    struct Settings {
        bool limit_time = false;
        bool limit_nodes = false;
        bool limit_depth = false;
        bool infinite = false;
        TimeRep allowed_time = 0;
        uint64_t allowed_nodes = 0;
        Depth allowed_depth = 0;

        int moves_to_go = 0;
        TimeRep wtime = DEFAULT_SEARCH_TIME;
        TimeRep btime = DEFAULT_SEARCH_TIME;
        TimeRep winc = 0;
        TimeRep binc = 0;
    };

    // static vars in search thread?
    inline Settings settings = {};
    inline TimeRep start_time = 0;
    inline TimeRep time_limit = 0;
    inline std::atomic<bool> run = false;
    inline std::atomic<Depth> mt_depth = 0;

    // uci options
    inline bool chess960 = DEFAULT_CHESS960; // each board still has its own value with which it was constructed
    inline bool use_book = DEFAULT_USE_BOOK;
    inline bool show_currline = DEFAULT_SHOW_CURRLINE;
    inline bool show_currmove = DEFAULT_SHOW_CURRMOVE;
    inline Depth plies_muted = DEFAULT_MUTE_PLIES;
    inline int multi_pv = DEFAULT_MULTI_PV;
    inline TimeRep move_overhead = DEFAULT_OVERHEAD;
    inline TimeRep update_interval = DEFAULT_UPDATE_INTERVAL;
    inline TimeRep last_update_time = 0;

    // also static vars in search thread?
    inline TranspositionTable tt;
    inline std::vector<std::unique_ptr<SearchThread>> threads;

    void Init();
    void StartSearch(const Settings& settings, const Board &board);
    void FinalizeSearch();
    void Shutdown();

    [[nodiscard]] bool EnoughTimeLeft();
    [[nodiscard]] bool IsSearchLimited();
    [[nodiscard]] uint64_t NodesTotal();
    [[nodiscard]] inline bool Run() { return run.load(std::memory_order_relaxed); }
    inline void WaitFinished() { std::ranges::for_each(threads, &SearchThread::WaitForFinish); }
    inline void StopSearch() { run = false; }
    inline void ClearTT() { tt.Clear(); }
    inline void ShowShowCurrLine(bool show) { show_currline = show; }
    inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
    inline void SetPliesMuted(Depth ply_muted) { plies_muted = std::clamp(ply_muted, MIN_MUTE_PLIES, MAX_MUTE_PLIES); }
    inline void SetMultiPv(int pv_num) { multi_pv = std::clamp(pv_num, MIN_MULTI_PV, MAX_MULTI_PV); }
    inline void SetTTSize(int size_mb) { tt.Init(size_mb); }
    inline void SetUseBook(bool use) { use_book = use; }
    inline void SetChess960(bool set) { chess960 = set; }
    inline void SetMoveOverhead(TimeRep overhead) { move_overhead = std::clamp(overhead, MIN_OVERHEAD, MAX_OVERHEAD); }
    inline void SetUpdateInterval(TimeRep interval) {
        update_interval = std::clamp(interval, MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL);
    }
    void SetNumThreads(int num_threads);
}

#endif //MEETRA_SEARCH_H
