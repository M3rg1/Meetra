#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"
#include <stack>

namespace Meetra {

    class ABSearch {

    public:
        ABSearch();
        void StartSearch(const Board &b, Depth max_depth, long allowed_time);
        [[nodiscard]] std::string GetSearchInfo();
        [[nodiscard]] std::string GetBestMove() const;
        [[nodiscard]] std::string GetCurrMoveInfo() const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;
        void RetrievePv(Move *pv_line, Depth depth);
        void ResizeTT(TTSize size);
        void ClearTT();

        inline void StopSearch() { run = false; }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:
        void InitSearch();
        Score QuiescenceSearch(Score alpha, Score beta, Depth depth);
        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply);
        [[nodiscard]] bool EnoughTimeLeft(long allowed_time) const;
        [[nodiscard]] long ElapsedTimeMs() const;


        TranspositionTable tt;
        Board board;

        volatile bool run;
        Timer search_timer;
        Timer info_timer;

        struct MoveAndEval {
            Move move;
            Score score;
        };

        MoveAndEval move_evals[MAX_LEGAL_MOVES];

        //std::pair<Score, Move> score_move_pair[MAX_LEGAL_MOVES];
        int moves_count;
        Move curr_move;
        int curr_move_num;
        ulong nodes_searched;
        ulong qsearch_nodes;
        ulong tt_hits;
        Depth qsearch_depth;
        Depth curr_max_depth;
        long timer_start;
    };

}

#endif //MEETRA_SEARCH_H
