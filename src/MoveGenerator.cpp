#include "MoveGenerator.h"
#include "Bitboards.h"
#include "Board.h"
#include "Types.h"

namespace Meetra {

    template <Color C>
    constexpr Direction pawn_push_dir(){
        return C == WHITE ? NORTH : SOUTH;
    }

    template <Color C>
    constexpr Direction pawn_capture_left_dir(){
        return C == WHITE ? NORTH_WEST : SOUTH_EAST;
    }

    template <Color C>
    constexpr Direction pawn_capture_right_dir(){
        return C == WHITE ? NORTH_EAST : SOUTH_WEST;
    }

    template <Color C>
    constexpr Bitboard promotion_rank(){
        return C == WHITE ? 0xFF00000000000000UL : 0xFF000000000000FFUL;
    }

    template <Color C>
    constexpr Bitboard two_fwd_rank(){
        return C == WHITE ? 0x00000000FF000000UL : 0x000000FF00000000UL;
    }

    template<PieceType PT, Color C>
    inline void GenMovesForPieceType(const Board &board, Bitboard occ, std::deque<Move> &d, Bitboard legality_mask) {
        Bitboard pieces = board.GetPieces(PT, C);
        while (pieces) {
            Square origin_s = PopLsb(pieces);
            Bitboard possible_moves = GetAttacksForPiece<PT>(origin_s, occ) & legality_mask;
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                d.emplace_back(NewMove(origin_s, destination_s));
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
                    d.emplace_back(NewMove(E1, G1, CASTLING));
                }
            }
            if (board.CanWhiteLongCR()) {
                Bitboard empty_squares = SquareToBB(B1) | SquareToBB(C1) | SquareToBB(D1);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(C1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(D1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E1, BLACK, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E1, C1, CASTLING));
                }
            }
        } else {
            if (board.CanBlackShortCR()) {
                Bitboard empty_squares = SquareToBB(F8) | SquareToBB(G8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(F8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(G8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E8, G8, CASTLING));
                }
            }
            if (board.CanBlackLongCR()) {
                Bitboard empty_squares = SquareToBB(B8) | SquareToBB(C8) | SquareToBB(D8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.SquareAttackers(C8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(D8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.SquareAttackers(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E8, C8, CASTLING));
                }
            }
        }
    }

    template<Direction D>
    constexpr Bitboard shift(Bitboard b) {
        return  D == NORTH ? b << 8 : D == SOUTH ? b >> 8 : D == NORTH+NORTH? b <<16 : D == SOUTH+SOUTH? b >>16 :
        D == EAST ? (b & ~file_masks[FILE_H]) << 1 : D == WEST ? (b & ~file_masks[FILE_A]) >> 1 : D == NORTH_EAST ? (b & ~file_masks[FILE_H]) << 9 :
        D == NORTH_WEST ? (b & ~file_masks[FILE_A]) << 7 : D == SOUTH_EAST ? (b & ~file_masks[FILE_H]) >> 7 : D == SOUTH_WEST ? (b & ~file_masks[FILE_A]) >> 9 : 0;
    }

    template<Color C>
    inline void GenPawnForwardMoves(const Board &board, std::deque<Move> &d, Bitboard legality_mask) {

        constexpr Direction push_dir = pawn_push_dir<C>();
        Bitboard pawns_one_fw = shift<push_dir>(board.GetPieces(PAWN, C)) & board.GetEmptySquares();
        Bitboard pawns_two_fw = shift<push_dir>(pawns_one_fw) & board.GetEmptySquares() & two_fwd_rank<C>() & legality_mask;
        Bitboard pawn_prom = pawns_one_fw & legality_mask & promotion_rank<C>();
        pawns_one_fw &= legality_mask & ~promotion_rank<C>();

        while (pawns_one_fw) {
            Square dest_s = PopLsb(pawns_one_fw);
            d.emplace_back(NewMove(dest_s - push_dir, dest_s));
        }

        while (pawn_prom) {
            Square dest_s = PopLsb(pawn_prom);
            Square origin_s = dest_s - push_dir;
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_QUEEN));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_ROOK));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_BISHOP));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_KNIGHT));
        }

        while (pawns_two_fw) {
            Square dest_s = PopLsb(pawns_two_fw);
            d.emplace_back(NewMove(dest_s - push_dir - push_dir, dest_s, TWO_FORWARD));
        }
    }

    template<Color C>
    inline void GenPawnCaptures(const Board &board, std::deque<Move> &d, Bitboard legality_mask) {

        Bitboard pawns = board.GetPieces(PAWN, C);
        Bitboard enemy_pieces = board.GetPieces(OtherColor(C));

        constexpr Direction left_dir = pawn_capture_left_dir<C>();
        constexpr Direction right_dir = pawn_capture_right_dir<C>();

        Bitboard left_captures = shift<left_dir>(pawns) & enemy_pieces & legality_mask;
        Bitboard right_captures = shift<right_dir>(pawns) & enemy_pieces & legality_mask;

        Bitboard left_prom = left_captures & promotion_rank<C>();
        Bitboard right_prom = right_captures & promotion_rank<C>();

        left_captures &= ~promotion_rank<C>();
        right_captures &= ~promotion_rank<C>();

        while (left_captures) {
            Square dest_s = PopLsb(left_captures);
            d.emplace_back(NewMove(dest_s - left_dir, dest_s));
        }

        while (right_captures) {
            Square dest_s = PopLsb(right_captures);
            d.emplace_back(NewMove(dest_s - right_dir, dest_s));
        }

        while (left_prom) {
            Square dest_s = PopLsb(left_prom);
            Square origin_s = dest_s - left_dir;
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_QUEEN));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_ROOK));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_BISHOP));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_KNIGHT));
        }

        while (right_prom) {
            Square dest_s = PopLsb(right_prom);
            Square origin_s = dest_s - right_dir;
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_QUEEN));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_ROOK));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_BISHOP));
            d.emplace_back(NewMove(origin_s, dest_s, PROMOTE_KNIGHT));
        }

        if (board.EpSquare()) {
            Square ep_s = board.EpSquare();
            Bitboard attackers =
                    GetAttacksForPiece<PAWN>(ep_s, board.GetPieces(ALL_TYPES), OtherColor(C)) &
                    board.GetPieces(PAWN, C);
            while (attackers) {
                Square origin_s = PopLsb(attackers);
                d.emplace_back(NewMove(origin_s, ep_s, EN_PASSANT));
            }
        }
    }

// Ignoring pins, and king moving to attacked squares, and other quirky stuff like EP
    template<GenPhase phase, Color C>
    inline void GenMoves(const Board &board, std::deque<Move> &d) {

        Bitboard legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        Bitboard occ = board.GetPieces(ALL_TYPES);

        if (phase == EVASION) {
            if (PopCount(board.GetCheckers()) > 1) {
                GenMovesForPieceType<KING, C>(board, occ, d,
                                              board.GetPieces(OtherColor(C)) | board.GetEmptySquares());
            }
            return;
        }

        if (board.GetCheckers()) {
            if (PopCount(board.GetCheckers()) > 1) {
                return;
            }
            Square king_square = Lsb(board.GetPieces(KING, C));
            Bitboard capture_mask = board.GetCheckers();
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }

        Bitboard phase_mask = phase == CAPTURE ? board.GetPieces(OtherColor(C)) : board.GetEmptySquares();

        GenMovesForPieceType<KNIGHT, C>(board, occ, d, legal_moves & phase_mask);
        GenMovesForPieceType<BISHOP, C>(board, occ, d, legal_moves & phase_mask);
        GenMovesForPieceType<ROOK, C>(board, occ, d, legal_moves & phase_mask);
        GenMovesForPieceType<QUEEN, C>(board, occ, d, legal_moves & phase_mask);
        GenMovesForPieceType<KING, C>(board, occ, d, phase_mask);
        if (phase == QUIET) {
            GenCastlingMoves(board, d, C);
            GenPawnForwardMoves<C>(board, d, legal_moves);
        } else {
            GenPawnCaptures<C>(board, d, legal_moves);
        }

    }

    template void GenMoves<EVASION, WHITE>(const Meetra::Board &board, std::deque<Move> &d);
    template void GenMoves<CAPTURE, WHITE>(const Board &board, std::deque<Move> &d);
    template void GenMoves<QUIET, WHITE>(const Board &board, std::deque<Move> &d);
    template void GenMoves<EVASION, BLACK>(const Board &board, std::deque<Move> &d);
    template void GenMoves<CAPTURE, BLACK>(const Board &board, std::deque<Move> &d);
    template void GenMoves<QUIET, BLACK>(const Board &board, std::deque<Move> &d);

// gen moves all

}