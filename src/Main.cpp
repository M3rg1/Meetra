#include "Uci.h"
#include "Bitboards.h"
#include "ZobristHash.h"
#include "Search.h"

int main() {

    Uci::Init();
    //Bitboards::Init();
    Zobrist::Init();
    Search::Init();

    Uci::Listen();

    Search::Shutdown();

    return 0;
}
