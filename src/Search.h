#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"

namespace Meetra {

    class ABSearch {

    public:
        ABSearch();
        void StartSearch(Board &b, Depth max_depth, long allowed_time, int num_threads, bool fixed_timer);
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
        };

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
