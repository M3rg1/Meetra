#include "MoveGen.h"
#include <iostream>


namespace Meetra {

    /**
     In Joker (and qperft) I use only a partial legal-move generator. It will not generate moves with pinned pieces that leave
     you in check, by first determining which pieces are pinned, and then temporarily remove them from the piece list, and generate
     their moves along the pin line. This saves time in two ways: you don't waste time on generating invalid moves for the pinned pieces
     , and you don't have to test all other moves for King exposure.

    The King moves (incl. castlings) and e.p. captures that are generated are still pseudo-legal, though. Their legality was most
     efficiently checked only after the move was made (to prevent the King from stepping 'into its own shadow', and detecting
     the famous e.p. double-pin).
     */

    MoveGen::MoveGen(const Board &board, GenPhase start_phase) : board(board) {
        genPhase = start_phase;

        checkers = SquareAttackers(Lsb(board.GetPieces(KING, board.ColorToMove())),
                                   OtherColor(board.ColorToMove()), board.GetPieces(ALL_TYPES));

        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        if(PopCount(checkers) > 1){
            genPhase = EVASION;
        }
        else if (checkers) {
            Square king_square = Lsb(board.GetPieces(KING, board.ColorToMove()));
            Bitboard capture_mask = checkers;
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }
    }

    Move MoveGen::GetNextMove() {
        while (moves.empty()) {
            if (board.ColorToMove() == WHITE) {
                GenNewMoves<WHITE>();
            } else {
                GenNewMoves<BLACK>();
            }
        }

        // https://www.chessprogramming.org/Move_Ordering -- "Typical move ordering"
        // selection sort to pick the best move - pass through the whole list once and pick move with highest score

        Move m = moves.front();
        moves.pop_front();
        return m;
    }

    inline Bitboard MoveGen::SquareAttackers(Square s, Color attacked_by, Bitboard occ) const {
        return (GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & board.GetPieces(PAWN, attacked_by)) |
               (GetAttacksForPiece<KNIGHT>(s) & board.GetPieces(KNIGHT, attacked_by)) |
               (GetAttacksForPiece<BISHOP>(s, occ) & (board.GetPieces(BISHOP, attacked_by) | board.GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<ROOK>(s, occ) & (board.GetPieces(ROOK, attacked_by) | board.GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<KING>(s) & board.GetPieces(KING, attacked_by));
    }

    template<Color C>
    inline void MoveGen::GenNewMoves() {
        switch (genPhase) {
            case BEST_MOVE:
                ++genPhase;
                // return TT / killer move // or make case: Killer Move (also from history heuristic possible)
                // also null move? PV? etc.
                break;
            case CAPTURE:
                GenMoves<CAPTURE, C>(board, moves, legal_moves);
                ++genPhase;
                break;
            case QUIET:
                GenMoves<QUIET, C>(board, moves, legal_moves);
                ++genPhase;
                break;
            case END:
                moves.emplace_back(INVALID_MOVE);
                break;
            case EVASION:
                GenMoves<EVASION, C>(board, moves, legal_moves);
                genPhase = END;
                break;
        }
    }

    template<Color C>
    constexpr Direction pawn_push_dir() {
        return C == WHITE ? NORTH : SOUTH;
    }

    template<Color C>
    constexpr Direction pawn_capture_left_dir() {
        return C == WHITE ? NORTH_WEST : SOUTH_EAST;
    }

    template<Color C>
    constexpr Direction pawn_capture_right_dir() {
        return C == WHITE ? NORTH_EAST : SOUTH_WEST;
    }

    template<Color C>
    constexpr Bitboard promotion_rank() {
        return C == WHITE ? 0xFF00000000000000UL : 0xFF000000000000FFUL;
    }

    template<Color C>
    constexpr Bitboard two_fwd_rank() {
        return C == WHITE ? 0x00000000FF000000UL : 0x000000FF00000000UL;
    }

    template<PieceType PT, Color C>
    void GenMovesForPieceType(const Board &board, Bitboard occ, std::deque<Move> &d, Bitboard legality_mask) {
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

    template<Color C>
    void GenCastlingMoves(const Board &board, std::deque<Move> &d) {
        if (C == WHITE) {
            if (board.CanWhiteShortCR()) {
                Bitboard empty_squares = SquareToBB(F1) | SquareToBB(G1);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.IsSquareAttacked(F1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(G1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(E1, BLACK, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E1, G1, CASTLING));
                }
            }
            if (board.CanWhiteLongCR()) {
                Bitboard empty_squares = SquareToBB(B1) | SquareToBB(C1) | SquareToBB(D1);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.IsSquareAttacked(C1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(D1, BLACK, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(E1, BLACK, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E1, C1, CASTLING));
                }
            }
        } else {
            if (board.CanBlackShortCR()) {
                Bitboard empty_squares = SquareToBB(F8) | SquareToBB(G8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.IsSquareAttacked(F8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(G8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E8, G8, CASTLING));
                }
            }
            if (board.CanBlackLongCR()) {
                Bitboard empty_squares = SquareToBB(B8) | SquareToBB(C8) | SquareToBB(D8);
                if ((empty_squares & board.GetPieces(ALL_TYPES)) == EMPTY_BB &&
                    !board.IsSquareAttacked(C8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(D8, WHITE, board.GetPieces(ALL_TYPES)) &&
                    !board.IsSquareAttacked(E8, WHITE, board.GetPieces(ALL_TYPES))) {
                    d.emplace_back(NewMove(E8, C8, CASTLING));
                }
            }
        }
    }

    template<Direction D>
    constexpr Bitboard shift(Bitboard b) {
        return D == NORTH ? b << 8 : D == SOUTH ? b >> 8 : D == EAST ? (b & ~0x8080808080808080UL) << 1 :
                                                           D == WEST ? (b & ~0x0101010101010101UL) >> 1 : D == NORTH_EAST ? (b & ~0x8080808080808080UL) << 9 :
                                                                                                          D == NORTH_WEST  ? (b & ~0x0101010101010101UL) << 7 : D == SOUTH_EAST ? (b & ~0x8080808080808080UL) >> 7 :
                                                                                                                                                                D == SOUTH_WEST ? (b & ~0x0101010101010101UL) >> 9: 0;
    }

    template<Color C>
    void GenPawnForwardMoves(const Board &board, std::deque<Move> &d, Bitboard legality_mask) {

        constexpr Direction push_dir = pawn_push_dir<C>();
        Bitboard pawns_one_fw = shift<push_dir>(board.GetPieces(PAWN, C)) & board.GetEmptySquares();
        Bitboard pawns_two_fw =
                shift<push_dir>(pawns_one_fw) & board.GetEmptySquares() & two_fwd_rank<C>() & legality_mask;
        Bitboard pawn_prom = pawns_one_fw & legality_mask & promotion_rank<C>();
        pawns_one_fw &= legality_mask & ~promotion_rank<C>();

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

        while (pawns_one_fw) {
            Square dest_s = PopLsb(pawns_one_fw);
            d.emplace_back(NewMove(dest_s - push_dir, dest_s));
        }
    }

    template<Color C>
    void GenPawnCaptures(const Board &board, std::deque<Move> &d, Bitboard legality_mask) {

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

        while (left_captures) {
            Square dest_s = PopLsb(left_captures);
            d.emplace_back(NewMove(dest_s - left_dir, dest_s));
        }

        while (right_captures) {
            Square dest_s = PopLsb(right_captures);
            d.emplace_back(NewMove(dest_s - right_dir, dest_s));
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
    void GenMoves(const Board &board, std::deque<Move> &d, Bitboard legal_move_mask) {

        // rovnou bych tomu mohl rikat legal moves per square a initializovat to na legal_moves ( a az potom spocitat
        // piny a & s nima)
        // renmae the evasion phase to something like begin_phase ? or something more appropriate

        Bitboard occ = board.GetPieces(ALL_TYPES);

        if (phase == EVASION) {
            GenMovesForPieceType<KING, C>(board, occ, d, board.GetPieces(OtherColor(C))
                                                         | board.GetEmptySquares());
            return;
        }

        Bitboard phase_mask =
                phase == CAPTURE ? board.GetPieces(OtherColor(C)) : board.GetEmptySquares();

        GenMovesForPieceType<KNIGHT, C>(board, occ, d, legal_move_mask & phase_mask);
        GenMovesForPieceType<BISHOP, C>(board, occ, d, legal_move_mask & phase_mask);
        GenMovesForPieceType<ROOK, C>(board, occ, d, legal_move_mask & phase_mask);
        GenMovesForPieceType<QUEEN, C>(board, occ, d, legal_move_mask & phase_mask);
        GenMovesForPieceType<KING, C>(board, occ, d, phase_mask);
        if (phase == QUIET) {
            GenCastlingMoves<C>(board, d);
            GenPawnForwardMoves<C>(board, d, legal_move_mask);
        } else {
            GenPawnCaptures<C>(board, d, legal_move_mask);
        }
    }

/*    template void GenMoves<EVASION, WHITE>(const Meetra::Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);
    template void GenMoves<CAPTURE, WHITE>(const Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);
    template void GenMoves<QUIET, WHITE>(const Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);
    template void GenMoves<EVASION, BLACK>(const Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);
    template void GenMoves<CAPTURE, BLACK>(const Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);
    template void GenMoves<QUIET, BLACK>(const Board &board, std::deque<Move> &d, Bitboard checkers, Bitboard legal_move_mask);*/

}
