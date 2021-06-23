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

    MoveList moveList(board);
    auto m = moveList.GetNextMove();

    Move m1 = NewMove(B2, B4);
    board.MakeMove(m1);
    Move m2 = NewMove(H7, D5);
    board.MakeMove(m2);

    Move m3 = NewMove(D1, D7);
    if(!board.MakeMove(m3)){
        DEBUG_LOG("FALSE RETURNED");
    }


    board.UnmakeMove(m3);
    board.UnmakeMove(m2);
    board.UnmakeMove(m1);

    DEBUG_LOG(board.GetCheckers());
    DEBUG_LOG(board.PPBoard());
/*    DEBUG_LOG(PPBitboard(board.GetPieces(WHITE)));
    DEBUG_LOG(PPBitboard(board.GetPieces(ALL_TYPES)));
    DEBUG_LOG(board.PPBoard());*/


/*    board.UnmakeMove(m1);
    DEBUG_LOG(board.PPBoard());*/


#endif
    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
