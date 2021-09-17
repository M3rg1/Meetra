#include "Evaluator.h"
#include "Bitboards.h"
#include "EvalValues.h"
#include "Board.h"
#include <algorithm>

namespace Meetra::Evaluation {

    void Evaluator::SetBoard(const Board &board) {

        std::ranges::fill(mg, 0);
        std::ranges::fill(eg, 0);
        phase = 0;

        for (Color c = WHITE; c < COLOR_NR; ++c) {
            for (PieceType pt = PAWN; pt < PIECE_TYPE_NR; ++pt) {
                Bitboard pieces = board.GetPieces(pt, c);
                while (pieces) {
                    Square s = Bitboards::PopLsb(pieces);
                    mg[c] += EvalValues::mg_table[c][pt][s];
                    eg[c] += EvalValues::eg_table[c][pt][s];
                    phase += EvalValues::phase_inc[pt];
                }
            }
        }

        Color to_move = board.ColorToMove();
        mg_score = mg[to_move] - mg[OtherColor(to_move)];
        eg_score = eg[to_move] - eg[OtherColor(to_move)];
        mg_phase = std::min(phase, 24);
        eg_phase = 24 - mg_phase;
    }

    void Evaluator::MakeMove(const Board &board, Move m) {

        Color col = board.ColorToMove();
        Color enemy_col = OtherColor(col);

        Square to = ToSquare(m);
        Square from = FromSquare(m);
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (col == WHITE ? to + SOUTH : to + NORTH) : to;

        PieceType moved_pt = board.GetPieceTypeOnSq(from);
        PieceType taken_pt = board.GetPieceTypeOnSq(capture_s);

        mg[col] += EvalValues::mg_table[col][moved_pt][to] - EvalValues::mg_table[col][moved_pt][from];
        eg[col] += EvalValues::eg_table[col][moved_pt][to] - EvalValues::eg_table[col][moved_pt][from];

        if (taken_pt != NONE_PIECE_TYPE) {
            mg[enemy_col] -= EvalValues::mg_table[enemy_col][taken_pt][capture_s];
            eg[enemy_col] -= EvalValues::eg_table[enemy_col][taken_pt][capture_s];
            phase -= EvalValues::phase_inc[taken_pt];
        }

        if (IsPromotion(m)) {
            PieceType prom_to = PieceTypeFromFlag(GetMoveType(m));
            mg[col] += EvalValues::mg_table[col][prom_to][to] - EvalValues::mg_table[col][PAWN][to];
            eg[col] += EvalValues::eg_table[col][prom_to][to] - EvalValues::eg_table[col][PAWN][to];
            phase += EvalValues::phase_inc[prom_to] - EvalValues::phase_inc[PAWN];
        } else if (GetMoveType(m) == CASTLING) {
            Move r_move = board.RookCastlingMove(to, col);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            mg[col] += EvalValues::mg_table[col][ROOK][r_to] - EvalValues::mg_table[col][ROOK][r_from];
            eg[col] += EvalValues::eg_table[col][ROOK][r_to] - EvalValues::eg_table[col][ROOK][r_from];
        }

        mg_score = mg[enemy_col] - mg[col];
        eg_score = eg[enemy_col] - eg[col];
        mg_phase = std::min(phase, 24);
        eg_phase = 24 - mg_phase;
    }

    Score Evaluator::GetBoardEval() const {
        return (mg_score * mg_phase + eg_score * eg_phase) / 24;
    }

    Score Evaluator::GetMoveEval(const Board &board, Move m) const {

        Color col = board.ColorToMove();

        Square to = ToSquare(m);
        Square from = FromSquare(m);
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (col == WHITE ? to + SOUTH : to + NORTH) : to;

        PieceType moved_pt = board.GetPieceTypeOnSq(from);
        PieceType taken_pt = board.GetPieceTypeOnSq(capture_s);

        Score mg_val = EvalValues::mg_table[col][moved_pt][to] - EvalValues::mg_table[col][moved_pt][from];
        Score eg_val = EvalValues::eg_table[col][moved_pt][to] - EvalValues::eg_table[col][moved_pt][from];

        if (taken_pt != NONE_PIECE_TYPE) {
            mg_val += EvalValues::mg_table[OtherColor(col)][taken_pt][capture_s];
            eg_val += EvalValues::eg_table[OtherColor(col)][taken_pt][capture_s];
        }

        if (IsPromotion(m)) {
            PieceType prom_to = PieceTypeFromFlag(GetMoveType(m));
            mg_val += EvalValues::mg_table[col][prom_to][to] - EvalValues::mg_table[col][PAWN][to];
            eg_val += EvalValues::eg_table[col][prom_to][to] - EvalValues::eg_table[col][PAWN][to];
        } else if (GetMoveType(m) == CASTLING) {
            Move r_move = board.RookCastlingMove(to, col);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            mg_val += EvalValues::mg_table[col][ROOK][r_to] - EvalValues::mg_table[col][ROOK][r_from];
            eg_val += EvalValues::eg_table[col][ROOK][r_to] - EvalValues::eg_table[col][ROOK][r_from];
        }

        return (mg_val * mg_phase + eg_val * eg_phase) / 24;
    }
}
