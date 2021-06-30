#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include <thread>

namespace Meetra{

    void StartSearch(Board board, int max_depth);
    void StopSearch();
    bool IsSearching();

}

#endif //MEETRA_SEARCH_H
