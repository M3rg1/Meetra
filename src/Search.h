#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "PVTable.h"
#include "Timer.h"

namespace Meetra {

    class ABSearch {

    public:
        ABSearch();
        void StartSearch(Board board, Depth max_depth, long allowed_time);
        [[nodiscard]] std::string GetFullInfo() const;
        [[nodiscard]] std::string GetBestMove() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateInfo() const;
        void ResizeTT(TTSize size);
        void ClearTT();

        inline void StopSearch() { run = false; }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:
        void InitSearch();
        Score QuiescenceSearch(Board &board, Score alpha, Score beta, Depth depth);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth);
        [[nodiscard]] bool NotEnoughTimeLeft(long allowed_time) const;


        TranspositionTable *tt;
        PVTable pv_table;

        volatile bool run;
        Timer search_timer;
        Timer info_timer;

        Move curr_move;
        int curr_move_num;
        Move best_move;
        Score best_score;
        ulong nodes_searched;
        ulong qsearch_nodes;
        ulong tt_hits;
        Depth qsearch_depth;
        Depth curr_depth;
        long timer_start;
        bool mate_found;
        Depth mate_depth;
    };

}

#endif //MEETRA_SEARCH_H
