#include "PVTable.h"
#include <cstring>

namespace Meetra {

    PVTable::PVTable() {
        Reset();
    }

    void PVTable::Reset() {
        current_root_move_num = 0;
        memset(pv_length, 0, sizeof(pv_length));
    }

    void PVTable::SetCurrentRoot(int num){
        current_root_move_num = num;
    }

    void PVTable::AddEntry(Move move, Depth depth) {
        table[current_root_move_num][depth] = move;
        //if(depth + 1 > pv_length[current_root_move_num]){
            pv_length[current_root_move_num] = depth + 1;
        //}
    }

    void PVTable::AddScore(Score score) {
        pv_scores[current_root_move_num] = score;
    }

    Move PVTable::ProbePv(int root_move_num, Depth depth) const {
        return table[root_move_num][depth];
    }

    Score PVTable::ProbeScore(int root_move_num) const {
        return pv_scores[root_move_num];
    }

    bool PVTable::HasNext() const {
        return pv_length[current_root_move_num];
    }

}
