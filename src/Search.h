#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"
#include "Misc.h"

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
        [[nodiscard]] std::string GetBestMove() const;
        [[nodiscard]] std::string GetCurrMoveInfo(Move move, int num, Board &board) const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;

        inline void SetNumThreads(int num_threads) {
            //omp_set_num_threads(std::min(MAX_SEARCH_THREADS, num_threads));
        }
        inline void ClearTT() { tt.Clear(); }
        inline void SetTTSize(size_t size_mb) { tt.Resize(size_mb); }
        inline void ShowShowCurrLine(bool show) { show_currline = show; }
        inline void SetPliesMuted(int ply_muted) { plies_muted = ply_muted; }
        inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
        inline void StopSearch() {
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
        void InitSearchTimer(Board &board);
        void InitSearch(SearchSettings &settings, Board &board);
        Score QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply);
        void RetrievePv(Board &board, Move *pv_line, Depth depth) const;
        void SortRootMoves();
        void GenRootMoves(Board &board);
        [[nodiscard]] bool EnoughTimeLeft() const;
        [[nodiscard]] long ElapsedTimeMs() const;

        TranspositionTable tt;
        volatile bool run;
        SearchSettings settings;
        Timer search_timer;
        Timer info_timer;
        bool show_currline;
        bool show_currmove;
        int plies_muted;
        int multi_pv;

        MoveAndEval root_moves[MAX_LEGAL_MOVES];
        int root_moves_cnt;
        ulong normal_nodes;
        ulong qsearch_nodes;
        Depth qsearch_depth;
        Depth curr_max_depth;
        long timer_start;
    };

}

#endif //MEETRA_SEARCH_H
