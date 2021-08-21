#include "MoveGen.h"
#include "Bitboards.h"
#include "Evaluation.h"
#include "Uci.h"

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

    MoveGen::MoveGen(const Board &board) : board(board) {

        moves_cnt = 0;
        my_color = board.ColorToMove();
        enemy_color = OtherColor(my_color);
        enemy_pieces = board.GetPieces(enemy_color);
        all_pieces = board.GetPieces(ALL_TYPES);
        empty_squares = board.GetEmptySquares();
        checkers = board.SquareAttackers(Bitboards::Lsb(board.GetPieces(KING, my_color)), enemy_color, all_pieces);
        king_square = Bitboards::Lsb(board.GetPieces(KING, my_color));
        blockers = board.PinnedPiecesForSquare(king_square, enemy_color);
        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        gen_phase = CAPTURE;
        double_check = false;

        if (IsKingInCheck()) {
            if (Bitboards::MoreThanOne(checkers)) {
                gen_phase = DOUBLE_CHECK;
                double_check = true;
                legal_moves = 0;
                return;
            }
            Bitboard capture_mask = checkers;
            Square attacker_square = Bitboards::Lsb(capture_mask);
            Bitboard block_mask = Bitboards::GetRayBetweenSquares(king_square, attacker_square);
            legal_moves = capture_mask | block_mask;
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
    Move MoveGen::GetBestMove() {
        while (Empty()) {
            if (my_color == WHITE) {
                NextPhase<WHITE, QSearch>();
            } else {
                NextPhase<BLACK, QSearch>();
            }
            EvalMoves();
        }
        return PickBestMove();
    }

    Move MoveGen::GetAnyMove() {
        while (Empty()) {
            if (my_color == WHITE) {
                NextPhase<WHITE, false>();
            } else {
                NextPhase<BLACK, false>();
            }
        }
        return PopMove();
    }

    template Move MoveGen::GetBestMove<true>();
    template Move MoveGen::GetBestMove<false>();

    template<Color C, bool QSearch>
    void MoveGen::NextPhase() {
        switch (gen_phase) {
            case CAPTURE:
                GenMovesForPhase<CAPTURE, C>();
                gen_phase = QSearch ? END : QUIET;
                break;
            case QUIET:
                GenMovesForPhase<QUIET, C>();
                gen_phase = END;
                break;
            case END:
                PutMove(ZERO_MOVE);
                break;
            case DOUBLE_CHECK:
                GenMovesForPhase<DOUBLE_CHECK, C>();
                gen_phase = END;
                break;
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
            GenPawnCaptures<C, LEFT>();
            GenPawnCaptures<C, RIGHT>();
            GenEnPassantMoves<C>();
            GenMovesForPieceType<KING, C>(enemy_pieces);
        } else if constexpr (phase == DOUBLE_CHECK) {
            GenMovesForPieceType<KING, C>(enemy_pieces | empty_squares);
            return;
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

    template<Color C, PawnMoveDir D>
    void MoveGen::GenPawnCaptures() {

        Bitboard pawns = board.GetPieces(PAWN, C);
        constexpr Direction capture_dir = D == LEFT ? PawnCaptureLeftDir<C>() : PawnCaptureRightDir<C>();
        Bitboard captures = BitShift<capture_dir>(pawns) & phase_mask & ~PromotionRank<C>();
        Bitboard promotions = BitShift<capture_dir>(pawns) & phase_mask & PromotionRank<C>();

        while (promotions) {
            Square dest_s = Bitboards::PopLsb(promotions);
            Square origin_s = dest_s - capture_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }
        while (captures) {
            Square dest_s = Bitboards::PopLsb(captures);
            Square origin_s = dest_s - capture_dir;
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

    bool MoveGen::IsPseudoLegal(Move m) const {

        if (m == ZERO_MOVE) {
            return false;
        }

        // there exists a piece on the origin square
        Square from = FromSquare(m);
        Piece moved_piece = board.GetPieceOnSquare(from);
        if (moved_piece == NO_PIECE) {
            return false;
        }

        // in double check - only king moves are allowed
        PieceType moved_pt = TypeOfPiece(moved_piece);
        if (double_check && moved_pt != KING) {
            return false;
        }

        // the moved piece is of our color
        Color col_to_move = board.ColorToMove();
        if (ColorOfPiece(moved_piece) != col_to_move) {
            return false;
        }

        // destination is not occupied by a friendly piece
        Piece dest_piece = board.GetPieceOnSquare(ToSquare(m));
        if (dest_piece != NO_PIECE && ColorOfPiece(dest_piece) == col_to_move) {
            return false;
        }

        // castling validation
        MoveType move_type = GetMoveType(m);
        if (move_type == CASTLING) {
            return col_to_move == WHITE ? ValidateCastling<WHITE>(m) : ValidateCastling<BLACK>(m);
        }

        // ep validation
        if (move_type == EN_PASSANT) {
            if (moved_pt == PAWN && board.EpSquare()) {
                Square ep_s = board.EpSquare();
                Bitboard attacker = Bitboards::GetAttacksForPiece<PAWN>(ep_s, all_pieces, OtherColor(col_to_move)) &
                                    SquareToBB(from);
                if (attacker && m == NewMove(Bitboards::Lsb(attacker), ep_s, EN_PASSANT)) {
                    return true;
                }
            }
            return false;
        }

        // check if it's possible to generate a move that would match our tested move
        if (moved_pt == KING) return move_type == NO_FLAG && ValidateMoveForPiece<KING>(m);
        else if (moved_pt == QUEEN) return move_type == NO_FLAG && ValidateMoveForPiece<QUEEN>(m);
        else if (moved_pt == ROOK) return move_type == NO_FLAG && ValidateMoveForPiece<ROOK>(m);
        else if (moved_pt == BISHOP) return move_type == NO_FLAG && ValidateMoveForPiece<BISHOP>(m);
        else if (moved_pt == KNIGHT) return move_type == NO_FLAG && ValidateMoveForPiece<KNIGHT>(m);
        else if (moved_pt == PAWN) {
            return col_to_move == WHITE ? ValidatePawnMove<WHITE>(m) : ValidatePawnMove<BLACK>(m);
        }

        return false;
    }

    template<PieceType PT>
    bool MoveGen::ValidateMoveForPiece(Move m) const {
        Bitboard legality_mask = enemy_pieces | empty_squares;
        if constexpr (PT != KING) {
            legality_mask &= legal_moves;
        }
        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Bitboard possible_moves = Bitboards::GetAttacksForPiece<PT>(from, all_pieces) & legality_mask;
        if (blockers & SquareToBB(from)) {
            possible_moves &= Bitboards::GetRayBetweenEdges(king_square, from);
        }
        while (possible_moves) {
            if (to == Bitboards::PopLsb(possible_moves)) {
                return true;
            }
        }
        return false;
    }

    template<Color C>
    bool MoveGen::ValidatePawnMove(Move m) const {

        MoveType move_type = GetMoveType(m);

        if (move_type == NO_FLAG) {
            return HelperValidatePawnMove<C, LEFT, false>(m) || HelperValidatePawnMove<C, RIGHT, false>(m) ||
                   HelperValidatePawnMove<C, ONE_FWD, false>(m);
        }
        if (move_type == TWO_FORWARD) {
            return HelperValidatePawnMove<C, TWO_FWD, false>(m);
        }
        if (move_type == PROMOTE_QUEEN || move_type == PROMOTE_ROOK ||
            move_type == PROMOTE_BISHOP || move_type == PROMOTE_KNIGHT) {
            return HelperValidatePawnMove<C, LEFT, true>(m) || HelperValidatePawnMove<C, RIGHT, true>(m) ||
                   HelperValidatePawnMove<C, ONE_FWD, true>(m);
        }

        return false;
    }

    template<Color C, PawnMoveDir D, bool P>
    bool MoveGen::HelperValidatePawnMove(Move m) const {

        constexpr Direction move_dir = D == LEFT ? PawnCaptureLeftDir<C>() :
                D == RIGHT ? PawnCaptureRightDir<C>() :
                PawnFwdDir<C>();
        Bitboard moves = BitShift<move_dir>(SquareToBB(FromSquare(m)));
        moves &= D == LEFT || D == RIGHT ? enemy_pieces : empty_squares;
        moves &= P ? PromotionRank<C>() : ~PromotionRank<C>();
        moves = D == TWO_FWD ? BitShift<move_dir>(moves) & empty_squares & TwoFwdRank<C>() & legal_moves :
                moves & legal_moves;

        if (moves) {
            Square dest_s = Bitboards::Lsb(moves);
            if (ToSquare(m) == dest_s && !DiscoveryCheck(FromSquare(m), dest_s)) {
                return true;
            }
        }
        return false;
    }

    template<Color C>
    bool MoveGen::ValidateCastling(Move m) const {
        if (IsKingInCheck()) {
            return false;
        }
        if (CanCastleShort<C>(board.GetCR())) {
            if (m == NewMove(king_square, king_square + 2, CASTLING)) {
                return true;
            }
        }
        if (CanCastleLong<C>(board.GetCR())) {
            if (m == NewMove(king_square, king_square - 2, CASTLING)) {
                return true;
            }
        }
        return false;
    }
}
