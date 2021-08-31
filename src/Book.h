#ifndef MEETRA_BOOK_H
#define MEETRA_BOOK_H

#include <vector>
#include "ZobristHash.h"
#include <fstream>
#include <sstream>
#include "Uci.h"
#include "Board.h"
#include "MoveGen.h"
#include "Bitboards.h"
#include "BookKeys.h"
#include <execution>

namespace Meetra::Book {

    struct BookEntry {
        ZobristHash hash;
        Move move;

        BookEntry(ZobristHash h, Move m) : hash(h), move(m) {};
    };

    bool SaveBook(const std::vector<BookEntry> &book_entries);
    std::vector<BookEntry> RemoveBadPositions(std::vector<BookEntry> &positions);
    ZobristHash GenBookHash(const Board &board);
    std::vector<Move> ProbeBook(const Board &board);
    std::vector<BookEntry> ParsePgn();
    void CreateBook();
}

#endif //MEETRA_BOOK_H
