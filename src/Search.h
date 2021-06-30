#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include <thread>

namespace Meetra{

    void StartSearch(Board board, int max_depth, long allowed_time);
    void StopSearch();
    bool IsSearching();
    void InitSearch();

}

#endif //MEETRA_SEARCH_H
