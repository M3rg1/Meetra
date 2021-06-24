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

    InitBitboards();

    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;


#ifdef DEBUG_BUILD


    Move m1 = NewMove(B2, B6);
    board.MakeMove(m1);
    Move m2 = NewMove(H7, H5);
    board.MakeMove(m2);
    DEBUG_LOG(board.PPBoard());
    MoveList moveList(board);
    Move m;
    while ((m = moveList.GetNextMove()) != INVALID_MOVE) {
        DEBUG_LOG(GetMoveName(m));
    }


#endif
    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
