#include "MoveGen.h"


namespace Meetra {

    MoveGen::MoveGen(const Board &board, GenPhase start_phase) : board(board) {

        moves_cnt = 0;
        my_color = board.ColorToMove();
        enemy_color = OtherColor(my_color);
        enemy_pieces = board.GetPieces(enemy_color);
        all_pieces = board.GetPieces(ALL_TYPES);
        empty_squares = board.GetEmptySquares();

        checkers = board.SquareAttackers(Lsb(board.GetPieces(KING, my_color)),
                                         enemy_color, all_pieces);

        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        king_square = Lsb(board.GetPieces(KING, my_color));
        if (checkers) {
            if (PopCount(checkers) > 1) {
                genPhase = EVASION;
                return;
            }
            Bitboard capture_mask = checkers;
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }
        genPhase = start_phase;
        pinning_pieces = EMPTY_BB;
        blockers = board.PinnedPiecesForSquare(king_square, enemy_color, pinning_pieces);
    }

    Move MoveGen::GetNextMove() {
        while (Empty()) {
            if (my_color == WHITE) {
                NextPhase<WHITE>();
            } else {
                NextPhase<BLACK>();
            }
        }

        // https://www.chessprogramming.org/Move_Ordering -- "Typical move ordering"
        // selection sort to pick the best move - pass through the whole list once and pick move with highest score

        return PopMove();
    }

    template<Color C>
    void MoveGen::NextPhase() {
        switch (genPhase) {
            case BEST_MOVE:
                ++genPhase;
                // return TT / killer move // or make case: Killer Move (also from history heuristic possible)
                // also null move? PV? etc.
                break;
            case CAPTURE:
                GenMovesForPhase<CAPTURE, C>();
                ++genPhase;
                break;
            case QUIET:
                GenMovesForPhase<QUIET, C>();
                ++genPhase;
                break;
            case END:
                PutMove(INVALID_MOVE);
                break;
            case EVASION:
                GenMovesForPhase<EVASION, C>();
                genPhase = END;
                break;
        }
    }

    template<GenPhase phase, Color C>
    void MoveGen::GenMovesForPhase() {

        if constexpr (phase == EVASION) {
            GenMovesForPieceType<KING, C>(enemy_pieces | empty_squares);
            return;
        } else if constexpr (phase == QUIET) {
            phase_mask = legal_moves & empty_squares;
            GenPawnForwardMoves<C>();
            //Blockers_GenPawnForwardMoves<C>();
            GenCastlingMoves<C>();
            GenMovesForPieceType<KING, C>(empty_squares);
        } else if constexpr (phase == CAPTURE) {
            phase_mask = legal_moves & enemy_pieces;
            GenPawnCaptures<C>();
            //Blockers_GenPawnCaptures<C>();
            GenEnPassantMoves<C>();
            GenMovesForPieceType<KING, C>(enemy_pieces);
        }

        GenMovesForPieceType<KNIGHT, C>(phase_mask);
        GenMovesForPieceType<BISHOP, C>(phase_mask);
        GenMovesForPieceType<ROOK, C>(phase_mask);
        GenMovesForPieceType<QUEEN, C>(phase_mask);
    }

    template<PieceType PT, Color C>
    void MoveGen::GenMovesForPieceType(Bitboard legality_mask) {
        Bitboard pieces = board.GetPieces(PT, C);
        while (pieces) {
            Square origin_s = PopLsb(pieces);
            Bitboard possible_moves = GetAttacksForPiece<PT>(origin_s, all_pieces) & legality_mask;
            if (blockers & SquareToBB(origin_s)) {
                possible_moves &= rays_between_edges[king_square][origin_s];
            }
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                PutMove(NewMove(origin_s, destination_s));
            }
        }
    }

    template<Color C>
    void MoveGen::GenPawnForwardMoves() {

        constexpr Direction push_dir = pawn_push_dir<C>();
        Bitboard pawns_one_fw = shift<push_dir>(board.GetPieces(PAWN, C)) & empty_squares;
        Bitboard pawns_two_fw = shift<push_dir>(pawns_one_fw) & empty_squares & two_fwd_rank<C>() & phase_mask;
        Bitboard pawn_prom = pawns_one_fw & phase_mask & promotion_rank<C>();
        pawns_one_fw &= phase_mask & ~promotion_rank<C>();

        while (pawn_prom) {
            Square dest_s = PopLsb(pawn_prom);
            Square origin_s = dest_s - push_dir;
            if (blockers & SquareToBB(origin_s) && !(rays_between_edges[king_square][origin_s] & SquareToBB(dest_s))) {
                continue;
            }
            PutPromMoves(origin_s, dest_s);
        }

        while (pawns_two_fw) {
            Square dest_s = PopLsb(pawns_two_fw);
            Square origin_s = dest_s - push_dir - push_dir;
            if (blockers & SquareToBB(origin_s) && !(rays_between_edges[king_square][origin_s] & SquareToBB(dest_s))) {
                continue;
            }
            PutMove(NewMove(dest_s - push_dir - push_dir, dest_s, TWO_FORWARD));
        }

        while (pawns_one_fw) {
            Square dest_s = PopLsb(pawns_one_fw);
            Square origin_s = dest_s - push_dir;
            if (blockers & SquareToBB(origin_s) && !(rays_between_edges[king_square][origin_s] & SquareToBB(dest_s))) {
                continue;
            }
            PutMove(NewMove(dest_s - push_dir, dest_s));
        }
    }

    template<Color C>
    void MoveGen::GenPawnCaptures() {

        Bitboard pawns = board.GetPieces(PAWN, C);

        constexpr Direction left_dir = pawn_capture_left_dir<C>();
        constexpr Direction right_dir = pawn_capture_right_dir<C>();

        Bitboard left_captures =
                (shift<left_dir>(pawns & ~blockers) | (shift<left_dir>(pawns & blockers) & pinning_pieces)) &
                phase_mask;
        Bitboard right_captures =
                (shift<right_dir>(pawns & ~blockers) | (shift<right_dir>(pawns & blockers) & pinning_pieces)) &
                phase_mask;

        Bitboard left_prom = left_captures & promotion_rank<C>();
        Bitboard right_prom = right_captures & promotion_rank<C>();

        left_captures &= ~promotion_rank<C>();
        right_captures &= ~promotion_rank<C>();

        while (left_prom) {
            Square dest_s = PopLsb(left_prom);
            PutPromMoves(dest_s - left_dir, dest_s);
        }
        while (right_prom) {
            Square dest_s = PopLsb(right_prom);
            PutPromMoves(dest_s - right_dir, dest_s);
        }
        while (left_captures) {
            Square dest_s = PopLsb(left_captures);
            PutMove(NewMove(dest_s - left_dir, dest_s));
        }
        while (right_captures) {
            Square dest_s = PopLsb(right_captures);
            PutMove(NewMove(dest_s - right_dir, dest_s));
        }
    }

    template<Color C>
    void MoveGen::GenEnPassantMoves() {
        if (board.EpSquare()) {
            Square ep_s = board.EpSquare();
            Bitboard attackers = GetAttacksForPiece<PAWN>(ep_s, all_pieces, OtherColor(C)) & board.GetPieces(PAWN, C);
            while (attackers) {
                PutMove(NewMove(PopLsb(attackers), ep_s, EN_PASSANT));
            }
        }
    }

    template<Color C>
    void MoveGen::GenCastlingMoves() {
        if (checkers) {
            return;
        }
        if (CanCastleShort<C>(board.GetCR())) {
            PutMove(NewMove(king_square, king_square + 2, CASTLING));
        }
        if (CanCastleLong<C>(board.GetCR())) {
            PutMove(NewMove(king_square, king_square - 2, CASTLING));
        }
    }

    template<Color C>
    bool MoveGen::CanCastleShort(CastlingRights cr) {
        return cr & (C == WHITE ? WHITE_SHORT : BLACK_SHORT) &&
               (rays_between_squares[C == WHITE ? E1 : E8][C == WHITE ? H1 : H8] & all_pieces) == EMPTY_BB &&
               !board.IsSquareAttacked(C == WHITE ? F1 : F8, OtherColor(C), all_pieces);
    }

    template<Color C>
    bool MoveGen::CanCastleLong(CastlingRights cr) {
        return cr & (C == WHITE ? WHITE_LONG : BLACK_LONG) &&
               (rays_between_squares[C == WHITE ? E1 : E8][C == WHITE ? A1 : A8] & all_pieces) == EMPTY_BB &&
               !board.IsSquareAttacked(C == WHITE ? D1 : D8, OtherColor(C), all_pieces);
    }
}
