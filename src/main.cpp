#include <iostream>
#include "UciHandler.h"
#include "Bitboards.h"
#include "FenLoader.h"
#include "Board.h"
#include "Misc.h"
#include "Perft.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Meetra;


int main(int argc, char *arv[]) {

    InitBitboards();

    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;
    RunPerft(6, board);

    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
