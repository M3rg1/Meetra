#include <iostream>
#include "UciHandler.h"
#include "Bitboards.h"
#include "FenLoader.h"
#include "Board.h"
#include "Misc.h"
#include "Perft.h"
#include "Macros.h"
#include "MoveGen.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Meetra;


int main(int argc, char *arv[]) {

    // TODO make an automatic bulk perft testing so if something breaks and itsnt immediatelly visible we can know
    // when it happend, instead of finding out much later and having to figure it out backwards

    InitBitboards();

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    // r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
    // rnb1k2r/ppQ2ppp/1qp5/4b2B/1PB5/P5Nn/1PP3PP/RN2R1K1 b Q - 0 1     --- pins
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;

/*    MoveGen ml(board);
    board.MakeMove(ml.GetNextMove());*/

    RunPerft(5, board);

/*    INIT_TIMER
    START_TIMER
    for (int i = 0; i < 10000000; i++) {
        MoveGen ml(board);
        Move m;
        do {
            m = ml.GetNextMove();
        }while(m != INVALID_MOVE);
    }
    STOP_TIMER
    DEBUG_LOG(TIMER_GET_TIME_MS);

    START_TIMER
    for (int i = 0; i < 100000000; i++) {
        Move m = NewMove(A2, A3);
        board.MakeMove(m);
        board.UnmakeMove(m);
    }
    STOP_TIMER
    DEBUG_LOG(TIMER_GET_TIME_MS);*/


/*    ulong make_timer = 0UL;
    ulong unmake_timer = 0UL;
    ulong gen_timer = 0UL;
    ulong list_timer = 0UL;
    auto total_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 500000; i++) {
        MoveGen ml(board);

        START_TIMER
        Move m = ml.GetNextMove();;
        STOP_TIMER
        gen_timer += TIMER_GET_TIME_NS;

        while (m != INVALID_MOVE) {

            START_TIMER
            board.MakeMove(m);
            STOP_TIMER
            make_timer += TIMER_GET_TIME_NS;

            START_TIMER
            board.UnmakeMove(m);
            STOP_TIMER
            unmake_timer += TIMER_GET_TIME_NS;

            START_TIMER
            m = ml.GetNextMove();
            STOP_TIMER
            gen_timer += TIMER_GET_TIME_NS;
        }
    }
    auto total_start_end = std::chrono::high_resolution_clock::now();

    DEBUG_LOG("Make: " << (double) make_timer / 1000000);
    DEBUG_LOG("Unmake: " << (double) unmake_timer / 1000000);
    DEBUG_LOG("Gen: " << (double) gen_timer / 1000000);
    DEBUG_LOG("Move list: " << (double) list_timer / 1000000);
    DEBUG_LOG("Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(total_start_end-total_start).count());*/

    //UciHandler uciHandler;
    //uciHandler.Listen();

    return 0;
}
