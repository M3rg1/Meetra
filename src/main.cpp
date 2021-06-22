#include <iostream>
#include "UciHandler.h"
#include "Types.h"
#include "Macros.h"
#include "Bitboards.h"
#include "FenLoader.h"
#include "Board.h"
#include "Misc.h"
#include "MoveGenerator.h"
#include "MoveList.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Meetra;


int main(int argc, char *arv[]) {

    Board board("4k2r/6r1/8/8/8/8/3R4/R3K3 w Qk - 0 1");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;

    InitBitboards();

#ifdef DEBUG_BUILD



#endif
    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
