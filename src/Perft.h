#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include "Board.h"
#include "MoveGen.h"
#include "Macros.h"

namespace Meetra {

    int Perft(int depth, Board &board) {

        if (depth == 0) {
            return 1;
        }

        int nodes = 0;
        MoveGen moveGen(board);
        Move m;
        while ((m = moveGen.GetNextMove())) {
            if (board.MakeMove(m)) {
                nodes += Perft(depth - 1, board);
            }
            board.UnmakeMove(m);
        }

        return nodes;
    }

    void RunPerft(int depth, Board &board) {
        INIT_TIMER
        START_TIMER
        MoveGen moveGen(board);
        Move m;
        ulong total_nodes = 0;
        while ((m = moveGen.GetNextMove())) {
            if (board.MakeMove(m)) {
                int nodes = Perft(depth - 1, board);
                total_nodes += nodes;
                std::cout << GetMoveName(m) << ": " << nodes << std::endl;
            }
            board.UnmakeMove(m);
        }
        STOP_TIMER
        auto time_elapsed_ns = TIMER_GET_TIME_NS == 0 ? 1 : TIMER_GET_TIME_NS;
        auto nps = static_cast<ulong> (static_cast<double>(total_nodes) / (static_cast<double>(time_elapsed_ns) / 1000000000));
        std::cout << "Time elapsed: " << TIMER_GET_TIME_MS << "ms";
        std::cout << " | Nodes explored: " << total_nodes;
        std::cout << " | NPS: " << nps << std::endl;
    }

}


#endif //MEETRA_PERFT_H
