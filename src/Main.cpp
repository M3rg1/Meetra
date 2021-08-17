#include "Uci.h"
#include "Bitboards.h"
#include "ThreadPool.h"
#include "ZobristHash.h"
#include "Search.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Meetra;


int main(int argc, char *arv[]) {

    Uci::Init();
    ThreadPool::Init();
    Bitboards::Init();
    Zobrist::Init();
    Search::Init();


/*    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;
    */

    Uci::Listen();

    return 0;
}
