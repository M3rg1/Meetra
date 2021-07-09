#include "PVTable.h"

namespace Meetra {

    PVTable::PVTable() {
        count = 0;
    }

    void PVTable::Reset(){
        count = 0;
    }

    void PVTable::AddEntry(Move move) {
        table[count++] =  move;
    }

    Move PVTable::PopPV(){
        return table[--count];
    }

    bool PVTable::HasNext() const{
        return count;
    }

}
