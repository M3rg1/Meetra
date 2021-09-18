#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include <sstream>
#include <chrono>
#include "MoveGen.h"
#include "Uci.h"

namespace Meetra {

    inline uint64_t Perft(Depth depth, Board &board) {

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

    uint64_t RunPerft(Depth depth, Board &board, bool div = false) {

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
            if (div) {
                Uci::Send(board.MoveToName(m) + ": " + std::to_string(nodes));
            }
        }

        return total_nodes;
    }

}

#endif //MEETRA_PERFT_H
