#include <iostream>
#include "UciHandler.h"
#include "Types.h"
#include "Macros.h"
#include "Bitboards.h"
#include "FenLoader.h"
#include "Board.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Popper;


int main(int argc, char *arv[]) {

    //std::cout << argc << std::endl;
#ifdef DEBUG_BUILD

    Move m = NewMove(A4, G6, PROMOTE_KNIGHT);

    Board board("4k2r/6r1/8/8/8/8/3R4/R3K3 w Qk - 0 1");
    std::cout << board.PPBoard();

    //std::cout << TotalMoves(board.game_state) << std::endl;
    //std::cout << Ply(board.game_state) << std::endl;

/*    std::cout << li->full_move_count << std::endl;
    std::cout << li->color_to_move << std::endl;
    std::cout << li->ply << std::endl;
    std::cout << li->b_castle_long << std::endl;
    std::cout << li->ep_square << std::endl;*/

    Bitboard b = 1231000023;
    //std::cout << PPStringBitboard(b) << std::endl;

    //std::cout << (std::endian::native == std::endian::little) << std::endl;

#endif
    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
