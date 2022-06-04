#include "Uci.h"
#include "ZobristHash.h"
#include "Search.h"

int main() {

    Uci::Init();
    Zobrist::Init();
    Search::Init();

    Uci::Listen();

    Search::Shutdown();

    return 0;
}
