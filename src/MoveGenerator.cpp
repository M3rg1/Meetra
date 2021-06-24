#include "MoveGenerator.h"
#include "Bitboards.h"

namespace Meetra {

    template<PieceType PT>
    inline void GenMovesForPieceType(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {
        Bitboard pieces = board.GetPieces(PT, to_move);
        while (pieces) {
            Square origin_s = PopLsb(pieces);
            Bitboard possible_moves = GetAttacksForPiece<PT>(origin_s, board.GetPieces(ALL_TYPES), to_move);
            possible_moves &= legality_mask;
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s));
            }
        }
    }

    void GenPromotions(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {
        Bitboard promotion_mask = to_move == WHITE ? rank_masks[RANK_7] : rank_masks[RANK_2];
        Bitboard pawns = board.GetPieces(PAWN, to_move) & promotion_mask & legality_mask;
        while (pawns) {
            Square origin_s = PopLsb(pawns);
            Bitboard possible_moves = 0; // shift one
            possible_moves &= board.GetEmptySquares();
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_QUEEN));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_ROOK));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_BISHOP));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_KNIGHT));
            }
        }
        pawns = board.GetPieces(PAWN, to_move) & promotion_mask & legality_mask;
        while (pawns) {
            Square origin_s = PopLsb(pawns);
            Bitboard possible_moves = GetAttacksForPiece<PAWN>(origin_s, board.GetPieces(ALL_TYPES), to_move);
            possible_moves &= board.GetPieces(OtherColor(to_move));
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_QUEEN));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_ROOK));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_BISHOP));
                d.push_back(NewMove(origin_s, destination_s, PROMOTE_KNIGHT));
            }
        }
    }

    inline void GenPawnForwardMoves(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {
        Bitboard promotion_mask = to_move == WHITE ? ~rank_masks[RANK_7] : ~rank_masks[RANK_2];
        Bitboard pawns = board.GetPieces(PAWN, to_move) & promotion_mask & legality_mask;
        while (pawns) {
            Square origin_s = PopLsb(pawns);
            Bitboard possible_moves = 0; // shift one
            possible_moves &= board.GetEmptySquares();
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s));
            }
            // shift two
        }
    }

    inline void GenPawnCaptures(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {
        Bitboard promotion_mask = to_move == WHITE ? ~rank_masks[RANK_7] : ~rank_masks[RANK_2];
        Bitboard pawns = board.GetPieces(PAWN, to_move) & promotion_mask & legality_mask;
        while (pawns) {
            Square origin_s = PopLsb(pawns);
            Bitboard possible_moves = GetAttacksForPiece<PAWN>(origin_s, board.GetPieces(ALL_TYPES), to_move);
            possible_moves &= board.GetPieces(OtherColor(to_move));
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s));
            }
        }
        Square ep_s;
        if ((ep_s = board.EpSquare())) {
            Bitboard ep_mask = SquareToBB(ep_s);
            pawns = board.GetPieces(PAWN, to_move) & (to_move == WHITE ? rank_masks[RANK_5] : rank_masks[RANK_4]);
            while (pawns) {
                Square origin_s = PopLsb(pawns);
                Bitboard possible_moves = GetAttacksForPiece<PAWN>(origin_s, board.GetPieces(ALL_TYPES), to_move);
                possible_moves &= ep_mask;
                while (possible_moves) {
                    Square destination_s = PopLsb(possible_moves);
                    d.push_back(NewMove(origin_s, destination_s, EN_PASSANT));
                }
            }
        }
    }

    // Ignoring pins, and king moving to attacked squares, and other quirky stuff like EP
    template<GenPhase phase>
    inline void GenMoves(const Board &board, std::deque<Move> &d, Color to_move) {

        Bitboard legal_moves = 0xFFFFFFFFFFFFFFFFUL;

        if (board.GetCheckers()) {
            if (PopCount(board.GetCheckers()) > 1) {
                GenMovesForPieceType<KING>(board, d, to_move, board.GetPieces(OtherColor(to_move)));
                GenMovesForPieceType<KING>(board, d, to_move, board.GetEmptySquares());
                return;
            }
            Square king_square = Lsb(board.GetPieces(KING, to_move));
            Bitboard capture_mask = board.GetCheckers();
            Square attacker_square = Lsb(capture_mask);
            // if pawn is checking the king, the ray will be empty
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }

        switch (phase) {
            case PROMOTION:
                GenPromotions(board, d, to_move, legal_moves);
                break;
            case CAPTURE:
                // no king moves
                // when calcing captures, do only those that would take the checker, if we are in check
                // this should be easy enough -> moves & checkers in stead of moves & enemy pieces
                GenPawnForwardMoves(board, d, to_move, legal_moves);
                GenMovesForPieceType<KNIGHT>(board, d, to_move, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<BISHOP>(board, d, to_move, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<ROOK>(board, d, to_move, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<QUEEN>(board, d, to_move, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<KING>(board, d, to_move, board.GetPieces(OtherColor(to_move)) & legal_moves);
                break;
            case QUIET:
                // no need to generate king moves anymore here ??
                GenPawnCaptures(board, d, to_move, legal_moves);
                GenMovesForPieceType<KNIGHT>(board, d, to_move, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<BISHOP>(board, d, to_move, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<ROOK>(board, d, to_move, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<QUEEN>(board, d, to_move, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<KING>(board, d, to_move, board.GetEmptySquares() & legal_moves);
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