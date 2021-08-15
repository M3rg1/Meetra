#include "MoveGen.h"
#include "Bitboards.h"
#include "Evaluation.h"


namespace Meetra {


    template<Color C>
    constexpr Direction PawnFwdDir() {
        return C == WHITE ? NORTH : SOUTH;
    }

    template<Color C>
    constexpr Direction PawnCaptureLeftDir() {
        return C == WHITE ? NORTH_WEST : SOUTH_EAST;
    }

    template<Color C>
    constexpr Direction PawnCaptureRightDir() {
        return C == WHITE ? NORTH_EAST : SOUTH_WEST;
    }

    template<Color C>
    constexpr Bitboard PromotionRank() {
        return C == WHITE ? 0xFF00000000000000UL : 0xFF000000000000FFUL;
    }

    template<Color C>
    constexpr Bitboard TwoFwdRank() {
        return C == WHITE ? 0x00000000FF000000UL : 0x000000FF00000000UL;
    }

    template<Direction D>
    constexpr Bitboard BitShift(Bitboard b) {
        if constexpr (D == NORTH) return b << 8;
        else if constexpr (D == SOUTH) return b >> 8;
        else if constexpr (D == EAST) return (b & ~0x8080808080808080UL) << 1;
        else if constexpr (D == WEST) return (b & ~0x0101010101010101UL) >> 1;
        else if constexpr (D == NORTH_EAST) return (b & ~0x8080808080808080UL) << 9;
        else if constexpr (D == NORTH_WEST) return (b & ~0x0101010101010101UL) << 7;
        else if constexpr (D == SOUTH_EAST) return (b & ~0x8080808080808080UL) >> 7;
        else if constexpr (D == SOUTH_WEST) return (b & ~0x0101010101010101UL) >> 9;
        else return 0;
    }

    MoveGen::MoveGen(const Board &board, const TranspositionTable *tt) : board(board), tt(tt) {

        moves_cnt = 0;
        my_color = board.ColorToMove();
        enemy_color = OtherColor(my_color);
        enemy_pieces = board.GetPieces(enemy_color);
        all_pieces = board.GetPieces(ALL_TYPES);
        empty_squares = board.GetEmptySquares();
        checkers = board.SquareAttackers(Bitboards::Lsb(board.GetPieces(KING, my_color)), enemy_color, all_pieces);
        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        king_square = Bitboards::Lsb(board.GetPieces(KING, my_color));
        blockers = board.PinnedPiecesForSquare(king_square, enemy_color);
        genPhase = BEST_MOVE;

        if (IsKingInCheck()) {
            if (Bitboards::MoreThanOne(checkers)) {
                if (my_color == WHITE) {
                    GenMovesForPieceType<KING, WHITE>(enemy_pieces | empty_squares);
                } else {
                    GenMovesForPieceType<KING, BLACK>(enemy_pieces | empty_squares);
                }
                EvalMoves();
                genPhase = END;
                return;
            }
            Bitboard capture_mask = checkers;
            Square attacker_square = Bitboards::Lsb(capture_mask);
            Bitboard block_mask = Bitboards::GetRayBetweenSquares(king_square, attacker_square);
            legal_moves = capture_mask | block_mask;
        }
    }

    MoveGen::MoveGen(const Board &board) : MoveGen(board, nullptr) {
        if (genPhase != END) {
            genPhase = CAPTURE;
        }
    }

    void MoveGen::EvalMoves() {
        for (auto i = 0; i < moves_cnt; i++) {
            move_eval[i].score =
                    IsValidMove(move_eval[i].move) ? Evaluation::MoveEval(board, move_eval[i].move) : NEGATIVE_INF;
        }
    }

    Move MoveGen::PickBestMove() {
        auto it = std::max_element(move_eval, move_eval + moves_cnt, CompScoreLesserMAE);
        return PopRef(*it);
    }

    template<bool QSearch>
    Move MoveGen::GetNextMove() {
        while (Empty()) {
            if (my_color == WHITE) {
                NextPhase<WHITE, QSearch>();
            } else {
                NextPhase<BLACK, QSearch>();
            }
            EvalMoves();
            //std::sort(move_eval, move_eval + moves_cnt, CompScoreGreatersMAN);
        }
        //return PopMove();
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
                    m = tt->GetAnyMove(board.GetZobristHash());
                    if (IsValidMove(m)) {
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
            Square origin_s = Bitboards::PopLsb(pieces);
            Bitboard possible_moves = Bitboards::GetAttacksForPiece<PT>(origin_s, all_pieces) & legality_mask;
            if (blockers & SquareToBB(origin_s)) {
                possible_moves &= Bitboards::GetRayBetweenEdges(king_square, origin_s);
            }
            while (possible_moves) {
                Square destination_s = Bitboards::PopLsb(possible_moves);
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
            Square dest_s = Bitboards::PopLsb(pawn_prom);
            Square origin_s = dest_s - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }

        while (pawns_two_fw) {
            Square dest_s = Bitboards::PopLsb(pawns_two_fw);
            Square origin_s = dest_s - push_dir - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s, TWO_FORWARD));
            }
        }

        while (pawns_one_fw) {
            Square dest_s = Bitboards::PopLsb(pawns_one_fw);
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
            Square dest_s = Bitboards::PopLsb(left_prom);
            Square origin_s = dest_s - left_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }
        while (right_prom) {
            Square dest_s = Bitboards::PopLsb(right_prom);
            Square origin_s = dest_s - right_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }
        while (left_captures) {
            Square dest_s = Bitboards::PopLsb(left_captures);
            Square origin_s = dest_s - left_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
        while (right_captures) {
            Square dest_s = Bitboards::PopLsb(right_captures);
            Square origin_s = dest_s - right_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
    }

    // allow movement only on a line between piece and king, if piece is a blocker
    bool MoveGen::DiscoveryCheck(Square origin, Square destination) const {
        return (blockers & SquareToBB(origin)) &&
               !(Bitboards::GetRayBetweenEdges(king_square, origin) & SquareToBB(destination));
    }

    template<Color C>
    void MoveGen::GenEnPassantMoves() {
        if (board.EpSquare()) {
            Square ep_s = board.EpSquare();
            Bitboard attackers =
                    Bitboards::GetAttacksForPiece<PAWN>(ep_s, all_pieces, OtherColor(C)) & board.GetPieces(PAWN, C);
            while (attackers) {
                PutMove(NewMove(Bitboards::PopLsb(attackers), ep_s, EN_PASSANT));
            }
        }
    }

    template<Color C>
    void MoveGen::GenCastlingMoves() {
        if (IsKingInCheck()) {
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
    bool MoveGen::CanCastleShort(CastlingRights cr) const {
        return cr & (C == WHITE ? WHITE_SHORT : BLACK_SHORT) &&
               (Bitboards::GetRayBetweenSquares(C == WHITE ? E1 : E8, C == WHITE ? H1 : H8) & all_pieces) == EMPTY_BB &&
               !board.IsSquareAttacked(C == WHITE ? F1 : F8, OtherColor(C), all_pieces);
    }

    template<Color C>
    bool MoveGen::CanCastleLong(CastlingRights cr) const {
        return cr & (C == WHITE ? WHITE_LONG : BLACK_LONG) &&
               (Bitboards::GetRayBetweenSquares(C == WHITE ? E1 : E8, C == WHITE ? A1 : A8) & all_pieces) == EMPTY_BB &&
               !board.IsSquareAttacked(C == WHITE ? D1 : D8, OtherColor(C), all_pieces);
    }

    template Move MoveGen::GetNextMove<true>();
    template Move MoveGen::GetNextMove<false>();
}
