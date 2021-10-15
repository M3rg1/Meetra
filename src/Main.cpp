#include "Uci.h"
#include "Bitboards.h"
#include "ZobristHash.h"
#include "Search.h"
#include "EvalValues.h"

int main() {

    Uci::Init();
    Bitboards::Init();
    Zobrist::Init();
    Search::Init();
    Evaluation::Init();

    Uci::Listen();

    return 0;
}
