#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include <sstream>
#include <chrono>
#include "MoveGen.h"
#include "Uci.h"

namespace Meetra {

    inline uint64_t Perft(Depth depth, Board &board) noexcept {

        MoveGen move_gen(board);
        uint64_t nodes = 0;
        Move m;

        // bulk counting at the leaves
        if (depth == 1) {
            while ((m = move_gen.GetAnyMove())) {
                if (board.IsMoveLegal(m)) {
                    nodes++;
                }
            }
            return nodes;
        }

        while ((m = move_gen.GetAnyMove())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            nodes += Perft(depth - 1, board);
            board.UnmakeMove(m);
        }

        return nodes;
    }

    inline void RunPerft(Depth depth, Board &board) noexcept {

        auto start = std::chrono::steady_clock::now();

        MoveGen move_gen(board);
        Move m;
        uint64_t total_nodes = 0;

        while ((m = move_gen.GetAnyMove())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            uint64_t nodes = depth > 1 ? Perft(depth - 1, board) : 1;
            board.UnmakeMove(m);
            total_nodes += nodes;
            Uci::Send(board.MoveToName(m) + ": " + std::to_string(nodes));
        }

        auto end = std::chrono::steady_clock::now();
        auto time_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() + 1;
        auto nps = static_cast<uint64_t> (static_cast<double>(total_nodes) /
                                          (static_cast<double>(time_elapsed_ns) / 1000000000.0));

        std::ostringstream oss;
        oss << "\nTime elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
            << " | Nodes explored: " << total_nodes
            << " | NPS: " << nps;

        Uci::Send(oss.str());
    }

}


#endif //MEETRA_PERFT_H
