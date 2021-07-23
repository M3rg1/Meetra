#ifndef MEETRA_SEARCHTASK_H
#define MEETRA_SEARCHTASK_H

#include <vector>
#include "Search.h"

namespace Meetra {

    class SearchTask {

    public:
        SearchTask(int t_num, Board b, std::vector<Search::RootMove> moves) {
            thread_num = t_num;
            board = b;
            root_moves = std::move(moves);
            curr_rm = nullptr;
            curr_rm_num = 0;
            curr_depth = 0;
            best_rm_num = 0;
        }

        void Search();

    private:

        Score NegaMax(Score alpha, Score beta, Depth depth, Depth ply);
        Score QSearch(Score alpha, Score beta, Depth ply);

        [[nodiscard]] std::string GetSearchInfo();
        [[nodiscard]] std::string GetCurrMoveInfo();

        void RetrievePv(Move *pv_line, Depth depth);
        [[nodiscard]] bool MateInHorizon() const;
        [[nodiscard]] inline bool IsMainThread() const { return thread_num == 0; }

        int thread_num;
        Board board;
        std::vector<Search::RootMove> root_moves;
        Search::RootMove *curr_rm;
        int curr_rm_num;
        int best_rm_num;
        Depth curr_depth;
    };

}


#endif //MEETRA_SEARCHTASK_H
