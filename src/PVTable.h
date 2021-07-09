#ifndef MEETRA_PVTABLE_H
#define MEETRA_PVTABLE_H

#include "Types.h"
#include "ZobristHash.h"

namespace Meetra {

    class PVTable {

    public:
        PVTable();
        void AddEntry(Move move);
        Move PopPV();
        void Reset();
        [[nodiscard]] bool HasNext() const;

    private:
        Move table[MAX_SEARCH_DEPTH];
        size_t count;
    };

}


#endif //MEETRA_PVTABLE_H
