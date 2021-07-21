#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"
#include "Misc.h"
#include "PVTable.h"
#include <mutex>
#include <future>

namespace Meetra {

    class ABSearch {

    public:

        struct SearchSettings {
            Depth max_allowed_depth = DEFAULT_SEARCH_DEPTH;
            bool fixed_timer = false;
            bool infinite = false;
            long allowed_time = DEFAULT_SEARCH_TIME;

            long white_time = 0;
            long black_time = 0;
            long white_increment = 0;
            long black_increment = 0;

            long info_to_ui_ms_timer = DEFAULT_UI_SPAM;
        };

        ABSearch();
        void StartSearch(SearchSettings settings, Board board);
        [[nodiscard]] std::string GetSearchInfo(Board &board);
        [[nodiscard]] std::string GetCurrMoveInfo(Move move, int num, Board &board) const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;

        inline void SetNumThreads(int num) { num_threads = num; }
        inline void ClearTT() { tt.Clear(); pvt.Clear(); }
        inline void SetTTSize(size_t size_mb) { tt.Resize(size_mb);  pvt.Clear(); }
        inline void ShowShowCurrLine(bool show) { show_currline = show; }
        inline void SetPliesMuted(int ply_muted) { plies_muted = ply_muted; }
        inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
        inline void StopSearch() {
            std::scoped_lock lock(mtx);
            run = false;
            search_timer.Stop();
            info_timer.Stop();
        }
        inline void SetMultiPv(int pv_num) {
            if (pv_num < 1) return;
            multi_pv = pv_num;
        }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:
        void MainSearch(Board board, MoveAndNodes *moves, int thread_num);
        bool MateInHorizon(Depth curr_depth);
        std::vector<std::future<void>> threads_futures;
        std::vector<std::unique_ptr<MoveAndNodes[]>> roots;
        void StartParSearch(SearchSettings s, Board board);
        Score NegaMax_Proper(Board &board, Score alpha, Score beta, Depth depth, Depth ply, ulong &nodes);
        Score QuiescenceSearch_Proper(Board &board, Score alpha, Score beta, ulong &nodes, Depth depth);
        void PrepareParSearch();
        void TerminateSearchThreads();
        void StartSearchThread(Board board, Depth search_depth, Score alpha, Score beta);
        Score NegaMax_Helper(Board &board, Score alpha, Score beta, Depth depth, Depth ply, const std::shared_ptr<bool>& stop);
        Score QuiescenceSearch_Helper(Board &board, Score alpha, Score beta);

        void InitSearchTimer(Board &board);
        void InitSearch(SearchSettings &settings, Board &board);
        Score QuiescenceSearch(Board &board, Score alpha, Score beta, Depth ply, Depth &max_reached_ply, ulong &nodes);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply, Depth &max_reached_ply, ulong &nodes, Depth current_max, int thread_num);
        void RetrievePv(Board &board, Move *pv_line, Depth depth) const;
        void BackupPv(Board &board, Depth depth);
        void GenRootMoves(Board &board);
        [[nodiscard]] bool EnoughTimeLeft() const;
        [[nodiscard]] long ElapsedTimeMs() const;


        std::recursive_mutex mtx;
        std::vector<std::future<void>> futures;
        TranspositionTable tt;
        PVTable pvt;
        volatile bool run;
        SearchSettings settings;
        Timer search_timer;
        Timer info_timer;
        bool show_currline;
        bool show_currmove;
        int plies_muted;
        int multi_pv;
        int num_threads;

        MoveAndNodes root_moves[MAX_LEGAL_MOVES];
        int root_moves_cnt;
        ulong nodes_explored;
        Depth curr_max_depth;
        Depth qsearch_depth;
        long timer_start;
    };

}

#endif //MEETRA_SEARCH_H
