#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"

namespace Meetra {

    class ABSearch {

    public:

        struct SearchSettings {
            Board board;
            int multi_pv;
            Depth max_allowed_depth;
            bool fixed_timer;
            bool infinite;
            long allowed_time;

            long white_time;
            long black_time;
            long white_increment;
            long black_increment;

            long info_to_ui_ms_timer;
        };

        ABSearch();
        void StartSearch(SearchSettings settings);
        [[nodiscard]] std::string GetSearchInfo(Board &board);
        [[nodiscard]] std::string GetBestMove() const;
        [[nodiscard]] std::string GetCurrMoveInfo(Move move, int num) const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        void SetNumThreads(int num_threads);
        void ResizeTT(TTSize size);
        void ClearTT();

        inline void StopSearch() { run = false; }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:
        void InitSearchTimer();
        void InitSearch(SearchSettings &settings);
        Score QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply);
        void RetrievePv(Board &board, Move *pv_line, Depth depth);
        void SortRootNodes();
        void GenRootNodes();
        [[nodiscard]] bool EnoughTimeLeft() const;
        [[nodiscard]] long ElapsedTimeMs() const;


        TranspositionTable tt;

        volatile bool run;
        Timer search_timer;
        Timer info_timer;

        struct MoveAndEval {
            Move move;
            Score score;
        };


        SearchSettings settings;

        Score best_score;
        Move best_move;
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
