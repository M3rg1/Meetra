#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"

namespace Meetra {

    class ABSearch {

    public:
        ABSearch();
        void StartSearch(Board &b, Depth max_depth, long allowed_time, int num_threads);
        [[nodiscard]] std::string GetSearchInfo(Board & board);
        [[nodiscard]] std::string GetBestMove() const;
        [[nodiscard]] std::string GetCurrMoveInfo(Move move, int num)  const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        void RetrievePv(Board & board, Move *pv_line, Depth depth);
        void ResizeTT(TTSize size);
        void ClearTT();

        inline void StopSearch() { run = false; }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:
        std::string ExperimentalGetSearchInfo(Board &board, Depth depth);
        void ExperimentalStartSearch(Board &board, Depth max_depth, long allowed_time, int num_threads);
        void InitSearch(Board &board);
        Score QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply);
        void SortRootNodes();
        void GenRootNodes(Board &board);
        [[nodiscard]] bool EnoughTimeLeft(long allowed_time) const;
        [[nodiscard]] long ElapsedTimeMs() const;


        TranspositionTable tt;

        volatile bool run;
        Timer search_timer;
        Timer info_timer;

        struct MoveAndEval {
            Move move;
            Score score;
            Depth depth_searched;
        };

        MoveAndEval root_moves[MAX_LEGAL_MOVES];
        int root_moves_cnt;

        //std::pair<Score, Move> score_move_pair[MAX_LEGAL_MOVES];
        ulong normal_nodes;
        ulong qsearch_nodes;
        ulong tt_hits;
        Depth qsearch_depth;
        Depth curr_max_depth;
        long timer_start;
    };

}

#endif //MEETRA_SEARCH_H
