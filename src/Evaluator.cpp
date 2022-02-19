#include <algorithm>
#include "Evaluator.h"
#include "Bitboards.h"
#include "EvalValues.h"
#include "Board.h"

using namespace Evaluation;

void Evaluator::SetBoard(const Board &board) {

    mg.fill(0);
    eg.fill(0);
    phase = 0;

    for (Color c: Colors) {
        for (PieceType pt: PieceTypes) {
            Bitboard pieces = board.Pieces(pt, c);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                mg[c] += mg_table[c][pt][s];
                eg[c] += eg_table[c][pt][s];
                phase += phase_inc[pt];
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
    Square capture_s = TypeOfMove(m) == EN_PASSANT ? EpCaptureSq(col, to) : to;
    PieceType moved_pt = board.PieceTypeOnSq(from);
    PieceType taken_pt = board.PieceTypeOnSq(capture_s);

    mg[col] += mg_table[col][moved_pt][to] - mg_table[col][moved_pt][from];
    eg[col] += eg_table[col][moved_pt][to] - eg_table[col][moved_pt][from];

    if (taken_pt != NONE_PIECE_TYPE) {
        mg[enemy_col] -= mg_table[enemy_col][taken_pt][capture_s];
        eg[enemy_col] -= eg_table[enemy_col][taken_pt][capture_s];
        phase -= phase_inc[taken_pt];
    }

    if (IsPromotion(m)) {
        PieceType prom_to = PromotionTo(TypeOfMove(m));
        mg[col] += mg_table[col][prom_to][to] - mg_table[col][PAWN][to];
        eg[col] += eg_table[col][prom_to][to] - eg_table[col][PAWN][to];
        phase += phase_inc[prom_to] - phase_inc[PAWN];
    } else if (TypeOfMove(m) == CASTLING) {
        Move r_move = board.RookCastlingMove(to, col);
        Square r_to = ToSquare(r_move);
        Square r_from = FromSquare(r_move);
        mg[col] += mg_table[col][ROOK][r_to] - mg_table[col][ROOK][r_from];
        eg[col] += eg_table[col][ROOK][r_to] - eg_table[col][ROOK][r_from];
    }

    mg_score = mg[enemy_col] - mg[col];
    eg_score = eg[enemy_col] - eg[col];
    mg_phase = std::min(phase, 24);
    eg_phase = 24 - mg_phase;
}

Score Evaluator::BoardEval() const {
    return (mg_score * mg_phase + eg_score * eg_phase) / 24;
}

Score Evaluator::MoveEval(const Board &board, Move m) const {

    Color col = board.ColorToMove();
    Square to = ToSquare(m);
    Square from = FromSquare(m);
    Square capture_s = TypeOfMove(m) == EN_PASSANT ? EpCaptureSq(col, to) : to;
    PieceType moved_pt = board.PieceTypeOnSq(from);
    PieceType taken_pt = board.PieceTypeOnSq(capture_s);

    Score mg_val = mg_table[col][moved_pt][to] - mg_table[col][moved_pt][from];
    Score eg_val = eg_table[col][moved_pt][to] - eg_table[col][moved_pt][from];

    if (taken_pt != NONE_PIECE_TYPE) {
        mg_val += mg_table[OtherColor(col)][taken_pt][capture_s];
        eg_val += eg_table[OtherColor(col)][taken_pt][capture_s];
    }

    if (IsPromotion(m)) {
        PieceType prom_to = PromotionTo(TypeOfMove(m));
        mg_val += mg_table[col][prom_to][to] - mg_table[col][PAWN][to];
        eg_val += eg_table[col][prom_to][to] - eg_table[col][PAWN][to];
    } else if (TypeOfMove(m) == CASTLING) {
        Move r_move = board.RookCastlingMove(to, col);
        Square r_to = ToSquare(r_move);
        Square r_from = FromSquare(r_move);
        mg_val += mg_table[col][ROOK][r_to] - mg_table[col][ROOK][r_from];
        eg_val += eg_table[col][ROOK][r_to] - eg_table[col][ROOK][r_from];
    }

    return (mg_val * mg_phase + eg_val * eg_phase) / 24;
}
