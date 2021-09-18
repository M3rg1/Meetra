#ifndef MEETRA_BOOK_H
#define MEETRA_BOOK_H

#include <vector>
#include "ZobristHash.h"
#include "Board.h"

namespace Meetra::Book {

    struct BookEntry {
        Hash64 hash;
        Move move;
    };

    std::vector<Move> ProbeBook(const Board &board);
    //void CreateBook();
}

#endif //MEETRA_BOOK_H
