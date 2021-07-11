#ifndef MEETRA_PVTABLE_H
#define MEETRA_PVTABLE_H

#include "Types.h"
#include "ZobristHash.h"

namespace Meetra {

    class PVTable {

    public:
        PVTable();
        void AddEntry(Move move, Depth depth);
        void AddScore(Score score);
        void SetCurrentRoot(int num);
        void Reset();

        [[nodiscard]] Score ProbeScore(int root_move_num) const;
        [[nodiscard]] Move ProbePv(int root_move_num, Depth depth) const;
        [[nodiscard]] bool HasNext() const;

    private:
        // roo moves = moves in table - have root moves pushed at the begining of search
        // and then we just take root moves from here when we run our search instead of generating them every time anew
        int current_root_move_num;
        Move table[128][MAX_SEARCH_DEPTH];
        Score pv_scores[128];
        Depth pv_length[128];
    };

}


#endif //MEETRA_PVTABLE_H
