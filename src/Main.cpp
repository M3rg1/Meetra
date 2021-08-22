#include "Uci.h"
#include "Bitboards.h"
#include "ZobristHash.h"
#include "Search.h"

int main(int argc, char *arv[]) {

    Meetra::Uci::Init();
    Meetra::Bitboards::Init();
    Meetra::Zobrist::Init();
    Meetra::Search::Init();

    Meetra::Uci::Listen();

    return 0;
}
