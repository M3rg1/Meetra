#include "MoveGenerator.h"
#include "Bitboards.h"
#include "Board.h"
#include "Types.h"

namespace Meetra {

    Bitboard promotion_mask[COLOR_NR]{
            0xFF00000000000000UL, 0x00000000000000FFUL
    };

    template<PieceType PT>
    inline void GenMovesForPieceType(const Board &board, Color to_move, Bitboard occ, std::deque<Move> &d, Bitboard legality_mask) {
        Bitboard pieces = board.GetPieces(PT, to_move);
        while (pieces) {
            Square origin_s = PopLsb(pieces);
            Bitboard possible_moves = GetAttacksForPiece<PT>(origin_s, occ) & legality_mask;
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.push_back(NewMove(origin_s, destination_s));
            }
        }
    }

    inline void GenCastlingMoves(const Board &board, std::deque<Move> &d, Color to_move) {
        if (to_move == WHITE) {
            if (board.CanWhiteShortCR()) {
                Bitboard empty_squares = SquareToBB(F1) | SquareToBB(G1);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(F1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(G1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E1, BLACK, board.GetPieces(ALL_TYPES))) {
                    d.push_back(NewMove(E1, G1, CASTLING));
                }
            }
            if (board.CanWhiteLongCR()) {
                Bitboard empty_squares = SquareToBB(B1) | SquareToBB(C1) | SquareToBB(D1);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(C1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(D1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E1, BLACK, board.GetPieces(ALL_TYPES))) {
                    d.push_back(NewMove(E1, C1, CASTLING));
                }
            }
        } else {
            if (board.CanBlackShortCR()) {
                Bitboard empty_squares = SquareToBB(F8) | SquareToBB(G8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(F8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(G8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.push_back(NewMove(E8, G8, CASTLING));
                }
            }
            if (board.CanBlackLongCR()) {
                Bitboard empty_squares = SquareToBB(B8) | SquareToBB(C8) | SquareToBB(D8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(C8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(D8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.push_back(NewMove(E8, C8, CASTLING));
                }
            }
        }
    }

/*    template<Direction D>
    constexpr Bitboard shift(Bitboard b) {
        return  D == NORTH ? b << 8 : D == SOUTH ? b >> 8 : D == NORTH+NORTH? b <<16 : D == SOUTH+SOUTH? b >>16 :
        D == EAST ? (b & ~FileHBB) << 1 : D == WEST ? (b & ~FileABB) >> 1 : D == NORTH_EAST ? (b & ~FileHBB) << 9 :
        D == NORTH_WEST ? (b & ~FileABB) << 7 : D == SOUTH_EAST ? (b & ~FileHBB) >> 7 : D == SOUTH_WEST ? (b & ~FileABB) >> 9 : 0;
    }*/


    inline void GenPawnForwardMoves(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {

        Bitboard pawns_one_fw, pawns_two_fw, pawn_prom;
        Direction origin_dir;

        if (to_move == WHITE) {
            origin_dir = SOUTH;
            pawns_one_fw = (board.GetPieces(PAWN, to_move) << 8) & board.GetEmptySquares();
            pawns_two_fw = (pawns_one_fw << 8) & board.GetEmptySquares() & 0x00000000FF000000UL & legality_mask;
            pawn_prom = pawns_one_fw & legality_mask & 0xFF00000000000000UL;
            pawns_one_fw &= legality_mask & ~0xFF00000000000000UL;
        } else {
            origin_dir = NORTH;
            pawns_one_fw = (board.GetPieces(PAWN, to_move) >> 8) & board.GetEmptySquares();
            pawns_two_fw = (pawns_one_fw >> 8) & board.GetEmptySquares() & 0x000000FF00000000UL & legality_mask;
            pawn_prom = pawns_one_fw & legality_mask & 0x00000000000000FFUL;
            pawns_one_fw &= legality_mask & ~0x00000000000000FFUL;
        }

        while (pawns_one_fw) {
            Square dest_s = PopLsb(pawns_one_fw);
            d.push_back(NewMove(dest_s + origin_dir, dest_s));
        }

        while (pawn_prom) {
            Square dest_s = PopLsb(pawn_prom);
            Square origin_s = dest_s + origin_dir;
            d.push_back(NewMove(origin_s, dest_s, PROMOTE_QUEEN));
            d.push_back(NewMove(origin_s, dest_s, PROMOTE_ROOK));
            d.push_back(NewMove(origin_s, dest_s, PROMOTE_BISHOP));
            d.push_back(NewMove(origin_s, dest_s, PROMOTE_KNIGHT));
        }

        while (pawns_two_fw) {
            Square dest_s = PopLsb(pawns_two_fw);
            d.push_back(NewMove(dest_s + origin_dir + origin_dir, dest_s, TWO_FORWARD));
        }
    }

    inline void GenPawnCaptures(const Board &board, std::deque<Move> &d, Color to_move, Bitboard legality_mask) {

        Bitboard pawns = board.GetPieces(PAWN, to_move);
        Bitboard enemy_pieces = board.GetPieces(OtherColor(to_move));

        while (pawns) {
            Square origin_s = PopLsb(pawns);
            Bitboard possible_moves =
                    GetAttacksForPiece<PAWN>(origin_s, board.GetPieces(ALL_TYPES), to_move) & legality_mask &
                    enemy_pieces;
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                if (SquareToBB(destination_s) & promotion_mask[to_move]) {
                    d.push_back(NewMove(origin_s, destination_s, PROMOTE_QUEEN));
                    d.push_back(NewMove(origin_s, destination_s, PROMOTE_ROOK));
                    d.push_back(NewMove(origin_s, destination_s, PROMOTE_BISHOP));
                    d.push_back(NewMove(origin_s, destination_s, PROMOTE_KNIGHT));
                } else {
                    d.push_back(NewMove(origin_s, destination_s));
                }
            }
        }

        Square ep_s;
        if ((ep_s = board.EpSquare())) {
            Bitboard attackers =
                    GetAttacksForPiece<PAWN>(ep_s, board.GetPieces(ALL_TYPES), OtherColor(to_move)) &
                    board.GetPieces(PAWN, to_move);
            while (attackers) {
                Square origin_s = PopLsb(attackers);
                d.push_back(NewMove(origin_s, ep_s, EN_PASSANT));
            }
        }
    }

// Ignoring pins, and king moving to attacked squares, and other quirky stuff like EP
    template<GenPhase phase>
    inline void GenMoves(const Board &board, std::deque<Move> &d, Color to_move) {

        Bitboard legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        Bitboard occ = board.GetPieces(ALL_TYPES);

        if (board.GetCheckers()) {
            if (PopCount(board.GetCheckers()) > 1) {
                if (phase == EVASION) {
                    GenMovesForPieceType<KING>(board, to_move, occ, d,
                                               board.GetPieces(OtherColor(to_move)) | board.GetEmptySquares());
                }
                return;
            }
            Square king_square = Lsb(board.GetPieces(KING, to_move));
            Bitboard capture_mask = board.GetCheckers();
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }

        switch (phase) {
            case CAPTURE:
                // no king moves
                // when calcing captures, do only those that would take the checker, if we are in check
                // this should be easy enough -> moves & checkers in stead of moves & enemy pieces
                GenPawnCaptures(board, d, to_move, legal_moves);
                // TODO we make our mask = enemy pieces mask here and then we call GenAll method that will call the other submethods
                GenMovesForPieceType<KNIGHT>(board, to_move, occ, d, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<BISHOP>(board, to_move, occ, d, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<ROOK>(board, to_move, occ, d, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<QUEEN>(board, to_move, occ, d, board.GetPieces(OtherColor(to_move)) & legal_moves);
                GenMovesForPieceType<KING>(board, to_move, occ, d, board.GetPieces(OtherColor(to_move)));
                break;
            case QUIET:
                // no need to generate king moves anymore here ??
                GenCastlingMoves(board, d, to_move);
                GenPawnForwardMoves(board, d, to_move, legal_moves);
                GenMovesForPieceType<KNIGHT>(board, to_move, occ, d, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<BISHOP>(board, to_move, occ, d, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<ROOK>(board, to_move, occ, d, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<QUEEN>(board, to_move, occ, d, board.GetEmptySquares() & legal_moves);
                GenMovesForPieceType<KING>(board, to_move, occ, d, board.GetEmptySquares());
                break;
            default:
                return;
                // case gen<all> - recursively all itself with all the possible generations - evasion, promotion etc.
        }
    }

    template void GenMoves<EVASION>(const Board &board, std::deque<Move> &d, Color to_move);
    template void GenMoves<CAPTURE>(const Board &board, std::deque<Move> &d, Color to_move);
    template void GenMoves<QUIET>(const Board &board, std::deque<Move> &d, Color to_move);
// gen moves all

}