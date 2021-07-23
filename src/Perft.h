#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include "MoveGen.h"
#include "Uci.h"
#include <sstream>
#include <chrono>

namespace Meetra {

    inline ulong Perft(Depth depth, Board &board) {

        if (depth == 0) {
            return 1;
        }

        ulong nodes = 0;
        MoveGen moveGen(board);
        Move m;
        while ((m = moveGen.GetNextMove<false>())) {
            if (board.MakeMove(m)) {
                nodes += Perft(depth - 1, board);
            }
            board.UnmakeMove(m);
        }

        return nodes;
    }

    inline void RunPerft(Depth depth, Board &board) {

        auto start = std::chrono::high_resolution_clock::now();

        MoveGen moveGen(board);
        Move m;
        ulong total_nodes = 0;
        while ((m = moveGen.GetNextMove<false>())) {
            if (board.MakeMove(m)) {
                ulong nodes = Perft(depth - 1, board);
                total_nodes += nodes;
                Uci::SendToGui(GetMoveName(m) + ": " + std::to_string(nodes));
            }
            board.UnmakeMove(m);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto time_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        time_elapsed_ns = std::max(1l, time_elapsed_ns);
        auto nps = static_cast<ulong> (static_cast<double>(total_nodes) /
                                       (static_cast<double>(time_elapsed_ns) / 1000000000));

        std::stringstream ss;
        ss << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
           << " | Nodes explored: " << total_nodes
           << " | NPS: " << nps;

        Uci::SendToGui(ss.str());
    }

}


#endif //MEETRA_PERFT_H
