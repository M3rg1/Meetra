#include "MoveGen.h"
#include "Bitboards.h"
#include "Evaluation.h"
#include "Macros.h"
#include <iostream>


namespace Meetra {

    // TODO quiescence will stop searching when no captures, but leave king in check

    MoveGen::MoveGen(const Board &board, const TranspositionTable *tt) : board(board), tt(tt) {

        moves_cnt = 0;
        my_color = board.ColorToMove();
        enemy_color = OtherColor(my_color);
        enemy_pieces = board.GetPieces(enemy_color);
        all_pieces = board.GetPieces(ALL_TYPES);
        empty_squares = board.GetEmptySquares();
        checkers = board.SquareAttackers(Lsb(board.GetPieces(KING, my_color)), enemy_color, all_pieces);
        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        king_square = Lsb(board.GetPieces(KING, my_color));
        blockers = board.PinnedPiecesForSquare(king_square, enemy_color);
        genPhase = BEST_MOVE;

        if (checkers) {
            if (MoreThanOne(checkers)) {
                if (my_color == WHITE) {
                    GenMovesForPieceType<KING, WHITE>(enemy_pieces | empty_squares);
                } else {
                    GenMovesForPieceType<KING, BLACK>(enemy_pieces | empty_squares);
                }
                SortMoves();
                genPhase = END;
                return;
            }
            Bitboard capture_mask = checkers;
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }
    }

    MoveGen::MoveGen(const Board &board) : MoveGen(board, nullptr) {
        genPhase = CAPTURE;
    }

    void MoveGen::SortMoves() {
        for (int i = 0; i < moves_cnt; i++) {
            if (moves[i] == INVALID_MOVE) {
                move_evals[i] = NEGATIVE_INF;
            } else {
                move_evals[i] = MoveEval(board, moves[i]);
            }
        }
    }

    Move MoveGen::PickBestMove() {
        int idx_best_move = 0;
        int max_eval = NEGATIVE_INF;
        for (int i = 0; i < moves_cnt; i++) {
            if (move_evals[i] > max_eval) {
                max_eval = move_evals[i];
                idx_best_move = i;
            }
        }
        return PopAtIdx(idx_best_move);
    }

    template<bool QSearch>
    Move MoveGen::GetNextMove() {
        while (Empty()) {
            if (my_color == WHITE) {
                NextPhase<WHITE, QSearch>();
            } else {
                NextPhase<BLACK, QSearch>();
            }
            SortMoves();
        }// info depth 7 nodes 47020168 time 7558 nps 6221244 score cp -155 pv h7h6

        // TODO i can do this only once, after the moves are generated and create an additional array
        // TODO with all the evals, and then just return moves based on that array
        // TODO where i just walk through the whole array and pick the best move to pop
        return PickBestMove();
    }

    template<Color C, bool QSearch>
    void MoveGen::NextPhase() {
        if constexpr (QSearch) {
            switch (genPhase) {
                case CAPTURE:
                    GenMovesForPhase<CAPTURE, C>();
                    genPhase = END;
                    break;
                case END:
                    PutMove(INVALID_MOVE);
                    break;
                default:
                    genPhase = CAPTURE;
                    break;
            }
        } else {
            switch (genPhase) {
                case BEST_MOVE:
                    Move m;
                    m = tt->GetPVMove(board.GetZobristHash());
                    if (m) {
                        PutMove(m);
                    }
                    ++genPhase;
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
                default:
                    genPhase = END;
                    break;
            }
        }
    }

    template<GenPhase phase, Color C>
    void MoveGen::GenMovesForPhase() {

        if constexpr (phase == QUIET) {
            phase_mask = legal_moves & empty_squares;
            GenPawnForwardMoves<C>();
            GenCastlingMoves<C>();
            GenMovesForPieceType<KING, C>(empty_squares);
        } else if constexpr (phase == CAPTURE) {
            phase_mask = legal_moves & enemy_pieces;
            GenPawnCaptures<C>();
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
                possible_moves &= rays_between_board_edges[king_square][origin_s];
            }
            while (possible_moves) {
                Square destination_s = PopLsb(possible_moves);
                PutMove(NewMove(origin_s, destination_s));
            }
        }
    }

    template<Color C>
    void MoveGen::GenPawnForwardMoves() {

        constexpr Direction push_dir = PawnFwdDir<C>();
        Bitboard pawns_one_fw = BitShift<push_dir>(board.GetPieces(PAWN, C)) & empty_squares;
        Bitboard pawns_two_fw = BitShift<push_dir>(pawns_one_fw) & empty_squares & TwoFwdRank<C>() & phase_mask;
        Bitboard pawn_prom = pawns_one_fw & phase_mask & PromotionRank<C>();
        pawns_one_fw &= phase_mask & ~PromotionRank<C>();

        while (pawn_prom) {
            Square dest_s = PopLsb(pawn_prom);
            Square origin_s = dest_s - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }

        while (pawns_two_fw) {
            Square dest_s = PopLsb(pawns_two_fw);
            Square origin_s = dest_s - push_dir - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s, TWO_FORWARD));
            }
        }

        while (pawns_one_fw) {
            Square dest_s = PopLsb(pawns_one_fw);
            Square origin_s = dest_s - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
    }

    template<Color C>
    void MoveGen::GenPawnCaptures() {

        Bitboard pawns = board.GetPieces(PAWN, C);

        constexpr Direction left_dir = PawnCaptureLeftDir<C>();
        constexpr Direction right_dir = PawnCaptureRightDir<C>();

        Bitboard left_captures = BitShift<left_dir>(pawns) & phase_mask;
        Bitboard right_captures = BitShift<right_dir>(pawns) & phase_mask;

        Bitboard left_prom = left_captures & PromotionRank<C>();
        Bitboard right_prom = right_captures & PromotionRank<C>();

        left_captures &= ~PromotionRank<C>();
        right_captures &= ~PromotionRank<C>();

        while (left_prom) {
            Square dest_s = PopLsb(left_prom);
            Square origin_s = dest_s - left_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }
        while (right_prom) {
            Square dest_s = PopLsb(right_prom);
            Square origin_s = dest_s - right_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }
        while (left_captures) {
            Square dest_s = PopLsb(left_captures);
            Square origin_s = dest_s - left_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
        while (right_captures) {
            Square dest_s = PopLsb(right_captures);
            Square origin_s = dest_s - right_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
    }

    bool MoveGen::DiscoveryCheck(Square origin, Square destination) {
        return (blockers & SquareToBB(origin)) &&
               !(rays_between_board_edges[king_square][origin] & SquareToBB(destination));
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

    template Move MoveGen::GetNextMove<true>();
    template Move MoveGen::GetNextMove<false>();
}
