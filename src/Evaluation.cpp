#include "Evaluation.h"
#include "Bitboards.h"
#include "EvalValues.h"

namespace Meetra {


    int MoveEval(const Board &board, Move move) {
        int material_eval = MoveMaterialEval(board, move);
        int position_eval = MovePositionEval(board, move);
        return material_eval + position_eval;
    }

    int MoveMaterialEval(const Board &board, Move move) {
        PieceType taken_piece = board.GetPieceTypeOnSq(ToSquare(move));
        PieceType this_piece = board.GetPieceTypeOnSq(FromSquare(move));
        return piece_values[taken_piece] - piece_values[this_piece];
    }

    int MovePositionEval(const Board &board, Move move) {
        Square from = FromSquare(move);
        Square to = ToSquare(move);
        PieceType pt = board.GetPieceTypeOnSq(from);
        auto eval = eval_maps[board.ColorToMove()][pt][to] - eval_maps[board.ColorToMove()][pt][from];
        eval += MoveCastlingEval(board, move);
        return eval;
    }

    int MoveCastlingEval(const Board &board, Move move) {
        auto eval = 0;
        Square from = FromSquare(move);
        if (board.GetPieceTypeOnSq(from) == KING && GetMoveType(move) != CASTLING &&
            board.CanColorCastleAny(board.ColorToMove())) {
            eval -= 50;
        } else if (board.GetPieceTypeOnSq(from) == ROOK && board.CanColorCastleAny(board.ColorToMove())) {
            eval -= 30;
        }
        return eval;
    }

    int BoardEval(const Board &board) {
        int material_eval = BoardMaterialEval(board);
        int position_eval = BoardPositionEval(board, board.ColorToMove());
        return material_eval + position_eval;
    }

    int BoardMaterialEval(const Board &board) {
        int white_eval = 0;
        int black_eval = 0;
        for (PieceType pt = PAWN; pt < KING; ++pt) {
            Bitboard b = board.GetPieces(pt, WHITE);
            white_eval += PopCount(b) * piece_values[pt];
            b = board.GetPieces(pt, BLACK);
            black_eval += PopCount(b) * piece_values[pt];
        }
        int perspective = board.ColorToMove() == WHITE ? 1 : -1;
        return (white_eval - black_eval) * perspective;
    }

    int BoardPositionEval(const Board &board, Color c) {
        int white_eval = 0;
        int black_eval = 0;
        for (PieceType pt = PAWN; pt < KING; ++pt) {
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while (pieces) {
                Square s = PopLsb(pieces);
                white_eval += eval_maps[WHITE][pt][s];
            }
            pieces = board.GetPieces(pt, BLACK);
            while (pieces) {
                Square s = PopLsb(pieces);
                black_eval += eval_maps[BLACK][pt][s];
            }
        }
        int perspective = board.ColorToMove() == WHITE ? 1 : -1;
        return (white_eval - black_eval) * perspective;
    }

}
