#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "Evaluation.h"
#include "TranspositionTable.h"
#include "PVTable.h"
#include <vector>
#include <future>

namespace Meetra::Search {

#define MAX_SEARCH_DEPTH 128
#define DEFAULT_SEARCH_DEPTH 32
#define DEFAULT_SEARCH_TIME 1000
#define DEFAULT_INFO_INTERVAL 1000
#define DEFAULT_SEARCH_THREADS 1
#define MAX_SEARCH_THREADS 8
#define MIN_MATE_EVAL (MATE_SCORE - MAX_SEARCH_DEPTH)
#define DEFAULT_PLY_FOR_DRAW 75

    struct SearchSettings {
        Depth max_allowed_depth = DEFAULT_SEARCH_DEPTH;
        bool fixed_timer = false;
        bool infinite = false;
        long allowed_time = DEFAULT_SEARCH_TIME;

        long white_time = 0;
        long black_time = 0;
        long white_increment = 0;
        long black_increment = 0;

        long info_to_ui_ms_timer = DEFAULT_INFO_INTERVAL;
    };

    namespace Globals {

        inline bool run;
        inline TranspositionTable tt;
        inline PVTable pvt;
        inline SearchSettings settings;
        inline bool show_currline;
        inline bool show_currmove;
        inline int plies_muted;
        inline int multi_pv;
        inline int num_threads;
        inline Move main_move;
        inline long nodes_explored;
        inline Depth curr_max_depth;
        inline Depth seldepth;
        inline long timer_start;
        inline int plies_draw;

        inline std::vector<std::future<void>> search_results;
    }

    void Init();
    void StartSearch(SearchSettings settings, Board board);
    std::string GetUpdateSearchInfo();

    [[nodiscard]] long ElapsedTimeMs();
    [[nodiscard]] bool EnoughTimeLeft();
    [[nodiscard]] inline bool IsSearching() { return Globals::run; }
    inline void ShowShowCurrLine(bool show) { Globals::show_currline = show; }
    inline void SetPliesMuted(int ply_muted) { Globals::plies_muted = ply_muted; }
    inline void ShowCurrMoveInfo(bool show) { Globals::show_currmove = show; }
    inline void StopSearch() { Globals::run = false; }
    inline void SetMultiPv(int pv_num) { Globals::multi_pv = pv_num; }
    inline void SetNumThreads(int num) { Globals::num_threads = std::clamp(num, 1, MAX_SEARCH_THREADS); }
    inline void SetPliesDraw(int plies) { Globals::plies_draw = plies; }
    inline void ClearTT() {
        Globals::tt.Clear();
        Globals::pvt.Clear();
    }
    inline void SetTTSize(int size_mb) {
        Globals::tt.Resize(size_mb);
        Globals::pvt.Clear();
    }

    struct RootMove {
        Move move;
        Score score;
        Score previous_score;
        Depth seldepth;
        long nodes;

        explicit RootMove(Move m) {
            move = m;
            score = NEGATIVE_INF;
            previous_score = NEGATIVE_INF;
            seldepth = 0;
            nodes = 0;
        }

        bool operator==(const Move &m) const { return move == m; }
        bool operator<(const RootMove &mn) const {
            return mn.nodes != nodes ? mn.nodes < nodes : mn.score < score;
        }
/*            bool operator<(const RootMove& mn) const {
                return mn.score != score ? mn.score < score : mn.previous_score < previous_score;
            }*/
    };
}

#endif //MEETRA_SEARCH_H
