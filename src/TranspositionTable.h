#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Types.h"
#include "Bitboards.h"
#include "Board.h"

namespace Meetra {

    class TranspositionTable {

    public:
        int size;
        Move table[100000];
        void AddEntry();
        Move GetEntray();

    private:

    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
