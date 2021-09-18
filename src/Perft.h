#ifndef MEETRA_PERFT_H
#define MEETRA_PERFT_H

#include "MoveGen.h"
#include "Uci.h"

namespace Meetra {

    template<bool DIV>
    uint64_t Perft(Depth depth, Board &board) {

        MoveGen move_gen(board);
        uint64_t total_nodes = 0;
        Move m;

        if (depth <= 1) {
            while ((m = move_gen.GetAnyMove())) {
                if (board.IsMoveLegal(m)) {
                    total_nodes++;
                    if constexpr (DIV) {
                        Uci::Send(board.MoveToName(m) + ": " + std::to_string(1));
                    }
                }
            }
            return total_nodes;
        }

        while ((m = move_gen.GetAnyMove())) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            uint64_t nodes = Perft<false>(depth - 1, board);
            board.UnmakeMove(m);
            total_nodes += nodes;
            if constexpr (DIV) {
                Uci::Send(board.MoveToName(m) + ": " + std::to_string(nodes));
            }
        }

        return total_nodes;
    }

}

#endif //MEETRA_PERFT_H
