#include "Evaluation.h"
#include "Bitboards.h"
#include "EvalValues.h"

namespace Meetra {


    Score MoveEval(const Board &board, Move move) {
        Score material_eval = MoveMaterialEval(board, move);
        Score position_eval = MovePositionEval(board, move);
        return material_eval + position_eval;
    }

    Score MoveMaterialEval(const Board &board, Move move) {
        PieceType taken_piece = board.GetPieceTypeOnSq(ToSquare(move));
        PieceType this_piece = board.GetPieceTypeOnSq(FromSquare(move));
        return piece_values[taken_piece] - piece_values[this_piece];
    }

    Score MovePositionEval(const Board &board, Move move) {
        Square from = FromSquare(move);
        Square to = ToSquare(move);
        PieceType pt = board.GetPieceTypeOnSq(from);
        Score eval = eval_maps[board.ColorToMove()][pt][to] - eval_maps[board.ColorToMove()][pt][from];
        eval += MoveCastlingEval(board, move);
        return eval;
    }

    Score MoveCastlingEval(const Board &board, Move move) {
        Score eval = 0;
        Square from = FromSquare(move);
        if (board.GetPieceTypeOnSq(from) == KING && GetMoveType(move) != CASTLING &&
            board.CanColorCastleAny(board.ColorToMove())) {
            eval -= 50;
        } else if (board.GetPieceTypeOnSq(from) == ROOK && board.CanColorCastleAny(board.ColorToMove())) {
            eval -= 30;
        }
        return eval;
    }

    Score BoardEval(const Board &board) {
        Score material_eval = BoardMaterialEval(board);
        Score position_eval = BoardPositionEval(board);
        return material_eval + position_eval;
    }

    Score BoardMaterialEval(const Board &board) {
        Score white_eval = 0;
        Score black_eval = 0;
        for (PieceType pt = PAWN; pt < KING; ++pt) {
            Bitboard b = board.GetPieces(pt, WHITE);
            white_eval += PopCount(b) * piece_values[pt];
            b = board.GetPieces(pt, BLACK);
            black_eval += PopCount(b) * piece_values[pt];
        }
        return board.ColorToMove() == WHITE ? (white_eval - black_eval) : -(white_eval - black_eval);
    }

    Score BoardPositionEval(const Board &board) {
        Score white_eval = 0;
        Score black_eval = 0;
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
        return board.ColorToMove() == WHITE ? (white_eval - black_eval) : -(white_eval - black_eval);
    }

}
