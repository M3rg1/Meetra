#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include <sstream>
#include <chrono>
#include "MoveGen.h"
#include "Uci.h"

namespace Meetra {

    inline ulong Perft(Depth depth, Board &board) {

        if (depth == 0) {
            return 1;
        }

        ulong nodes = 0;
        MoveGen move_gen(board);
        Move m;
        while ((m = move_gen.GetAnyMove())) {
            if(!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            nodes += Perft(depth - 1, board);
            board.UnmakeMove(m);
        }

        return nodes;
    }

    inline void RunPerft(Depth depth, Board &board) {

        auto start = std::chrono::steady_clock::now();

        MoveGen move_gen(board);
        Move m;
        ulong total_nodes = 0;
        while ((m = move_gen.GetAnyMove())) {
            if(!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            ulong nodes = Perft(depth - 1, board);
            board.UnmakeMove(m);
            total_nodes += nodes;
            Uci::SendToGui(GetMoveName(m) + ": " + std::to_string(nodes));
        }

        auto end = std::chrono::steady_clock::now();
        auto time_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() + 1;
        auto nps = static_cast<ulong> (static_cast<double>(total_nodes) /
                                       (static_cast<double>(time_elapsed_ns) / 1000000000.0));

        std::ostringstream oss;
        oss << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
            << " | Nodes explored: " << total_nodes
            << " | NPS: " << nps;

        Uci::SendToGui(oss.str());
    }

}


#endif //MEETRA_PERFT_H
