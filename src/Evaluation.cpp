#include "Evaluation.h"
#include "Bitboards.h"

namespace Meetra {

    int piece_values[PIECE_TYPE_NR]{
            100, 320, 330, 500, 900, 20000
    };

    int EvalBoard(Board &board) {
        int white_eval = 0;
        int black_eval = 0;
        for (PieceType pt = PAWN; pt < KING; ++pt) {
            Bitboard b = board.GetPieces(pt, WHITE);
            white_eval += PopCount(b) * piece_values[pt];
            b = board.GetPieces(pt, BLACK);
            black_eval += PopCount(b) * piece_values[pt];
        }
        return board.ColorToMove() == WHITE ? white_eval - black_eval : -(white_eval - black_eval);
    }

}
