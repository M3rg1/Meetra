#include <iostream>
#include "UciHandler.h"
#include "Bitboards.h"
#include "Board.h"
#include "Misc.h"
#include "Search.h"
#include "Perft.h"
#include <thread>
#include "ThreadPool.h"
#include "ZobristHash.h"

//  Variables: snake_case
//  Function names: UpperCamelCase (unless its a accessor/mutator)
//  Types: UpperCamelCase
//  Constants: kPrefixedCamelCase


using namespace Meetra;


int main(int argc, char *arv[]) {

    // TODO make an automatic bulk perft testing so if something breaks and itsnt immediatelly visible we can know
    // when it happend, instead of finding out much later and having to figure it out backwards

    InitBitboards();
    InitZobrist();
    //ThreadPool::InitThreadPool(4);

    //thread_pool = new ThreadPool(10);

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    // r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
    // rnb1k2r/ppQ2ppp/1qp5/4b2B/1PB5/P5Nn/1PP3PP/RN2R1K1 b Q - 0 1     --- pins
    // 1k4nQ/prpp1p2/bq2p1p1/3PNb2/np2P3/2N4p/PPPBBPPP/R3K2R w KQ - 0 1
/*    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << GetLogo() << std::endl;
    std::cout << " v. " << GetVersion() << std::endl;
    std::cout << " Made by " << GetAuthor() << std::endl << std::endl;
    std::cout << board.PPBoard() << std::endl;

    std::cout << GenZobristHash(board) << std::endl;*/

/*    InitSearch();
    StartSearch(board, 2, INFINITE_TIMER);*/

    //ThreadPool::InitThreadPool(6);
    //ThreadPool::GetInstance()->~ThreadPool();
    //ThreadPool::Resize(2);
    //ThreadPool::InitThreadPool(3);
    UciHandler uciHandler;
    uciHandler.Listen();
    //auto wrapper = std::bind(std::mem_fn(&UciHandler::Listen), uciHandler);
    //ThreadPool::PushTask(wrapper);
    // ThreadPool::PushTask([&uciHandler]() { uciHandler.Listen(); });

    return 0;
}
