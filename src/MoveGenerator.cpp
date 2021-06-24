#include "MoveGenerator.h"
#include "Bitboards.h"

namespace Meetra {

    void GenPromotions(const Board &board, std::deque<Move> &d, Color to_move) {

    }

    template<PieceType PT>
    inline void GenMovesForPieceType(const Board &board, std::deque<Move> &d, Color to_move, Bitboard mask) {
        Bitboard b = board.GetPieces(PT, to_move);
        while (b) {
            Square origin_s = PopLsb(b);
            Bitboard possible_moves = GetAttacksForPiece<PT>(origin_s, board.GetPieces(ALL_TYPES));
            possible_moves &= mask;
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s));
            }
        }
    }

    template<GenPhase phase>
    inline void GenMoves(const Board &board, std::deque<Move> &d, Color to_move) {

        if (PopCount(board.GetCheckers()) > 1 && phase != EVASION) {
            return;
        }

        switch (phase) {
            case EVASION:
                if (PopCount(board.GetCheckers())) {
                    GenMovesForPieceType<KING>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                    GenMovesForPieceType<KING>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                    return;
                }
                break;
            case PROMOTION:
                GenPromotions(board, d, to_move);
                break;
            case CAPTURE:
                // no capture promotions here
                // no king moves
                // when calcing captures, do only those that would take the checker, if we are in check
                // this should be easy enough -> moves & checkers in stead of moves & enemy pieces
                //GenMovesForPieceType<PAWN>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<KNIGHT>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<BISHOP>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<ROOK>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<QUEEN>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<KING>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                break;
            case QUIET:
                // no move promotions here
                // no need to generate king moves anymore here ??
                //GenMovesForPieceType<PAWN>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                GenMovesForPieceType<KNIGHT>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                GenMovesForPieceType<BISHOP>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                GenMovesForPieceType<ROOK>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                GenMovesForPieceType<QUEEN>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                GenMovesForPieceType<KING>(board, d, to_move, ~board.GetPieces(ALL_TYPES));
                break;
                // case gen<all> - recursively all itself with all the possible generations - evasion, promotion etc.
        }
    }

    template void GenMoves<EVASION>(const Board &board, std::deque<Move> &d, Color to_move);
    template void GenMoves<CAPTURE>(const Board &board, std::deque<Move> &d, Color to_move);
    template void GenMoves<PROMOTION>(const Board &board, std::deque<Move> &d, Color to_move);
    template void GenMoves<QUIET>(const Board &board, std::deque<Move> &d, Color to_move);
    // gen moves all

}