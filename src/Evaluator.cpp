#include "Evaluator.h"
#include "Bitboards.h"
#include "EvalValues.h"
#include "Board.h"
#include <algorithm>

namespace Meetra::Evaluation {

    void Evaluator::SetBoard(const Board &board) {

        std::ranges::fill(mg, 0);
        std::ranges::fill(eg, 0);
        gamePhase = 0;

        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                mg[WHITE] += EvalValues::mg_table[WHITE][pt][s];
                eg[WHITE] += EvalValues::eg_table[WHITE][pt][s];
                gamePhase += EvalValues::gamephaseInc[pt];
            }
            pieces = board.GetPieces(pt, BLACK);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                mg[BLACK] += EvalValues::mg_table[BLACK][pt][s];
                eg[BLACK] += EvalValues::eg_table[BLACK][pt][s];
                gamePhase += EvalValues::gamephaseInc[pt];
            }
        }

        Color to_move = board.ColorToMove();

        mgScore = mg[to_move] - mg[OtherColor(to_move)];
        egScore = eg[to_move] - eg[OtherColor(to_move)];
        mgPhase = std::min(gamePhase, 24);
        egPhase = 24 - mgPhase;
    }

    void Evaluator::MakeMove(const Board &board, Move m) {

        Color to_move = board.ColorToMove();
        Color enemy_col = OtherColor(to_move);

        Square to = ToSquare(m);
        Square from = FromSquare(m);
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (to_move == WHITE ? to + SOUTH : to + NORTH) : to;

        PieceType this_piece = board.GetPieceTypeOnSq(from);
        PieceType taken_piece = board.GetPieceTypeOnSq(capture_s);

        mg[to_move] += EvalValues::mg_table[to_move][this_piece][to] - EvalValues::mg_table[to_move][this_piece][from];
        eg[to_move] += EvalValues::eg_table[to_move][this_piece][to] - EvalValues::eg_table[to_move][this_piece][from];

        mg[enemy_col] -= EvalValues::mg_table[enemy_col][taken_piece][capture_s];
        eg[enemy_col] -= EvalValues::eg_table[enemy_col][taken_piece][capture_s];
        gamePhase -= EvalValues::gamephaseInc[taken_piece];

        if (IsPromotion(m)) {
            PieceType prom_to = PieceTypeFromFlag(GetMoveType(m));
            mg[to_move] += EvalValues::mg_table[to_move][prom_to][to] - EvalValues::mg_table[to_move][PAWN][to];
            eg[to_move] += EvalValues::eg_table[to_move][prom_to][to] - EvalValues::eg_table[to_move][PAWN][to];
            gamePhase += EvalValues::gamephaseInc[prom_to];
            gamePhase -= EvalValues::gamephaseInc[PAWN];
        } else if (GetMoveType(m) == CASTLING) {
            Move r_move = board.RookCastlingMove(to, to_move);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            mg[to_move] += EvalValues::mg_table[to_move][ROOK][r_to] - EvalValues::mg_table[to_move][ROOK][r_from];
            eg[to_move] += EvalValues::eg_table[to_move][ROOK][r_to] - EvalValues::eg_table[to_move][ROOK][r_from];
        }

        mgScore = mg[enemy_col] - mg[to_move];
        egScore = eg[enemy_col] - eg[to_move];
        mgPhase = std::min(gamePhase, 24);
        egPhase = 24 - mgPhase;
    }

    Score Evaluator::GetBoardEval() const {
        return (mgScore * mgPhase + egScore * egPhase) / 24;
    }

    Score Evaluator::GetMoveEval(const Board &board, Move m) const {

        Color to_move = board.ColorToMove();
        Square to = ToSquare(m);
        Square from = FromSquare(m);
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (to_move == WHITE ? to + SOUTH : to + NORTH) : to;

        PieceType this_piece = board.GetPieceTypeOnSq(from);
        PieceType taken_piece = board.GetPieceTypeOnSq(capture_s);

        Score mg_val = EvalValues::mg_table[to_move][this_piece][to] - EvalValues::mg_table[to_move][this_piece][from] +
                       EvalValues::mg_table[OtherColor(to_move)][taken_piece][capture_s];

        Score eg_val = EvalValues::eg_table[to_move][this_piece][to] - EvalValues::eg_table[to_move][this_piece][from] +
                       EvalValues::eg_table[OtherColor(to_move)][taken_piece][capture_s];

        if (IsPromotion(m)) {
            PieceType prom_to = PieceTypeFromFlag(GetMoveType(m));
            mg_val += EvalValues::mg_table[to_move][prom_to][to];
            eg_val += EvalValues::eg_table[to_move][prom_to][to];
            mg_val -= EvalValues::mg_table[to_move][PAWN][to];
            eg_val -= EvalValues::eg_table[to_move][PAWN][to];
        } else if (GetMoveType(m) == CASTLING) {
            Move rook_move = board.RookCastlingMove(to, to_move);
            Square rook_to = ToSquare(rook_move);
            Square rook_from = FromSquare(rook_move);
            mg_val += EvalValues::mg_table[to_move][ROOK][rook_to] - EvalValues::mg_table[to_move][ROOK][rook_from];
            eg_val += EvalValues::eg_table[to_move][ROOK][rook_to] - EvalValues::eg_table[to_move][ROOK][rook_from];
        }

        return (mg_val * mgPhase + eg_val * egPhase) / 24;
    }

    void Evaluator::UndoMove(const Board &board, Move m) {

        Color to_move = board.ColorToMove();
        Color enemy_col = OtherColor(to_move);

        Square to = ToSquare(m);
        Square from = FromSquare(m);
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (to_move == WHITE ? to + SOUTH : to + NORTH) : to;

        PieceType this_piece = board.GetPieceTypeOnSq(from);
        PieceType taken_piece = board.GetPieceTypeOnSq(capture_s);

        mg[to_move] -= EvalValues::mg_table[to_move][this_piece][to] - EvalValues::mg_table[to_move][this_piece][from];
        eg[to_move] -= EvalValues::eg_table[to_move][this_piece][to] - EvalValues::eg_table[to_move][this_piece][from];

        mg[enemy_col] += EvalValues::mg_table[enemy_col][taken_piece][capture_s];
        eg[enemy_col] += EvalValues::eg_table[enemy_col][taken_piece][capture_s];
        gamePhase += EvalValues::gamephaseInc[taken_piece];

        if (IsPromotion(m)) {
            PieceType prom_to = PieceTypeFromFlag(GetMoveType(m));
            mg[to_move] -= EvalValues::mg_table[to_move][prom_to][to];
            eg[to_move] -= EvalValues::eg_table[to_move][prom_to][to];
            mg[to_move] += EvalValues::mg_table[to_move][PAWN][to];
            eg[to_move] += EvalValues::eg_table[to_move][PAWN][to];
            gamePhase -= EvalValues::gamephaseInc[prom_to];
            gamePhase += EvalValues::gamephaseInc[PAWN];
        } else if (GetMoveType(m) == CASTLING) {
            Move rook_move = board.RookCastlingMove(to, to_move);
            Square rook_to = ToSquare(rook_move);
            Square rook_from = FromSquare(rook_move);
            mg[to_move] -=
                    EvalValues::mg_table[to_move][ROOK][rook_to] - EvalValues::mg_table[to_move][ROOK][rook_from];
            eg[to_move] -=
                    EvalValues::eg_table[to_move][ROOK][rook_to] - EvalValues::eg_table[to_move][ROOK][rook_from];
        }

        mgScore = mg[enemy_col] - mg[to_move];
        egScore = eg[enemy_col] - eg[to_move];
        mgPhase = std::min(gamePhase, 24);
        egPhase = 24 - mgPhase;
    }
}
