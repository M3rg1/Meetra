#include "MoveGen.h"
#include "Bitboards.h"
#include "Uci.h"

namespace Meetra {

    template<Color C, PawnMoveDir DIR>
    constexpr Direction PawnMove() {
        return C == WHITE ? DIR == LEFT ? NORTH_WEST : DIR == RIGHT ? NORTH_EAST : NORTH :
               DIR == LEFT ? SOUTH_EAST : DIR == RIGHT ? SOUTH_WEST : SOUTH;
    }

    template<Color C>
    constexpr Bitboard PromRank() {
        return C == WHITE ? 0xFF00000000000000UL : 0xFF000000000000FFUL;
    }

    template<Color C>
    constexpr Bitboard TwoFwdRank() {
        return C == WHITE ? 0x00000000FF000000UL : 0x000000FF00000000UL;
    }

    MoveGen::MoveGen(const Board &board) : board(board) {

        my_color = board.ColorToMove();
        enemy_color = OtherColor(my_color);
        all_pieces = board.GetPieces(ALL_TYPES);
        king_square = Bitboards::Lsb(board.GetPieces(KING, my_color));
        checkers = board.AttackedBy(king_square, enemy_color, all_pieces);
        blockers = board.PinnedToSquare(king_square, enemy_color);
        enemy_pieces = board.GetPieces(enemy_color);
        empty_squares = ~all_pieces;
        ep_s = board.EpSquare();
        moves_cnt = 0;
        legal_moves = 0xFFFFFFFFFFFFFFFFUL;

        if (IsKingInCheck()) {
            if (Bitboards::MoreThanOne(checkers)) {
                legal_moves = 0UL;
                gen_phase = DOUBLE_CHECK;
                double_check = true;
                return;
            }
            Bitboard capture_mask = checkers;
            Square attacker_square = Bitboards::Lsb(capture_mask);
            Bitboard block_mask = Bitboards::GetRayBetweenSquares(king_square, attacker_square);
            legal_moves = capture_mask | block_mask;
        }

        gen_phase = CAPTURE;
        double_check = false;
    }

    void MoveGen::EvalMoves() {
        for (size_t i = 0; i < moves_cnt; i++) {
            move_eval[i].score = move_eval[i].move != ZERO_MOVE ? board.GetMoveEval(move_eval[i].move) : NEGATIVE_INF;
        }
    }

    Move MoveGen::PickBestMove() {
        auto it = std::max_element(move_eval, move_eval + moves_cnt);
        return PopRef(*it);
    }

    template<GenType Type>
    Move MoveGen::GetBestMove() {
        while (Empty()) {
            my_color == WHITE ? NextPhase<WHITE, Type>() : NextPhase<BLACK, Type>();
            EvalMoves();
        }
        return PickBestMove();
    }

    Move MoveGen::GetAnyMove() {
        while (Empty()) {
            my_color == WHITE ? NextPhase<WHITE, NORMAL>() : NextPhase<BLACK, NORMAL>();
        }
        return PopMove();
    }

    template Move MoveGen::GetBestMove<QSEARCH>();

    template Move MoveGen::GetBestMove<NORMAL>();

    template<Color C, GenType Type>
    void MoveGen::NextPhase() {
        switch (gen_phase) {
            case CAPTURE:
                GenMovesForPhase<CAPTURE, C>();
                gen_phase = Type == QSEARCH ? END : QUIET;
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

        if constexpr (phase == DOUBLE_CHECK) {
            GenMovesForPieceType<KING, C>(enemy_pieces | empty_squares);
            return;
        } else if constexpr (phase == QUIET) {
            phase_mask = legal_moves & empty_squares;
            GenCastlingMoves<C>();
            GenPawnForwardMoves<C>();
            GenMovesForPieceType<KING, C>(empty_squares);
        } else if constexpr (phase == CAPTURE) {
            phase_mask = legal_moves & enemy_pieces;
            GenPawnCaptures<C, LEFT>();
            GenPawnCaptures<C, RIGHT>();
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
            Bitboard possible_moves = Bitboards::GetAttacks<PT>(origin_s, all_pieces) & legality_mask;
            if (blockers & SquareToBB(origin_s)) {
                possible_moves &= Bitboards::GetRayToBorders(king_square, origin_s);
            }
            while (possible_moves) {
                Square destination_s = Bitboards::PopLsb(possible_moves);
                PutMove(NewMove(origin_s, destination_s));
            }
        }
    }

    template<Color C>
    void MoveGen::GenPawnForwardMoves() {

        constexpr Direction push_dir = PawnMove<C, ONE_FWD>();
        Bitboard pawns = board.GetPieces(PAWN, C);
        Bitboard one_fwd = Bitboards::Shift<push_dir>(pawns) & empty_squares;
        Bitboard two_fwd = Bitboards::Shift<push_dir>(one_fwd) & empty_squares & TwoFwdRank<C>() & phase_mask;
        Bitboard promotions = one_fwd & phase_mask & PromRank<C>();
        one_fwd &= phase_mask & ~PromRank<C>();

        while (promotions) {
            Square dest_s = Bitboards::PopLsb(promotions);
            Square origin_s = dest_s - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutPromMoves(origin_s, dest_s);
            }
        }

        while (two_fwd) {
            Square dest_s = Bitboards::PopLsb(two_fwd);
            Square origin_s = dest_s - push_dir - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s, TWO_FORWARD));
            }
        }

        while (one_fwd) {
            Square dest_s = Bitboards::PopLsb(one_fwd);
            Square origin_s = dest_s - push_dir;
            if (!DiscoveryCheck(origin_s, dest_s)) {
                PutMove(NewMove(origin_s, dest_s));
            }
        }
    }

    template<Color C, PawnMoveDir D>
    void MoveGen::GenPawnCaptures() {

        constexpr Direction capture_dir = PawnMove<C, D>();
        Bitboard captures = Bitboards::Shift<capture_dir>(board.GetPieces(PAWN, C)) & phase_mask;
        Bitboard promotions = captures & PromRank<C>();
        captures &= ~PromRank<C>();

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
    bool MoveGen::DiscoveryCheck(Square orig, Square dest) const {
        return (blockers & SquareToBB(orig)) && !(Bitboards::GetRayToBorders(king_square, orig) & SquareToBB(dest));
    }

    template<Color C>
    void MoveGen::GenEnPassantMoves() {
        if (ep_s) {
            Bitboard attackers = Bitboards::GetAttacks<PAWN>(ep_s, EMPTY_BB, OtherColor(C)) & board.GetPieces(PAWN, C);
            while (attackers) {
                Square from = Bitboards::PopLsb(attackers);
                PutMove(NewMove(from, ep_s, EN_PASSANT));
            }
        }
    }

    template<Color C>
    void MoveGen::GenCastlingMoves() {
        if (IsKingInCheck()) {
            return;
        }
        if (CanCastle<C, SHORT>()) {
            PutMove(NewMove(king_square, C == WHITE ? G1 : G8, CASTLING));
        }
        if (CanCastle<C, LONG>()) {
            PutMove(NewMove(king_square, C == WHITE ? C1 : C8, CASTLING));
        }
    }

    template<Color C, CastlingSide S>
    bool MoveGen::CanCastle() const {

        Bitboard rook_bb = board.RookSqBB(C, S);
        if (!rook_bb) {
            return false;
        }

        // (for chess 960) we need to calculate all the squares that we travel through and make sure they are empty
        Square r_dest = S == LONG ? C == WHITE ? D1 : D8 : C == WHITE ? F1 : F8;
        Square k_dest = S == LONG ? C == WHITE ? C1 : C8 : C == WHITE ? G1 : G8;
        Bitboard pieces = all_pieces ^ rook_bb ^ SquareToBB(king_square);
        Bitboard walk_sq = Bitboards::GetRayBetweenSquares(Bitboards::Lsb(rook_bb), r_dest) |
                           Bitboards::GetRayBetweenSquares(king_square, k_dest) |
                           SquareToBB(r_dest) | SquareToBB(k_dest);

        return (pieces & walk_sq) == EMPTY_BB;
    }


    // this function does not guarantee the move is actually pseudo legal, it only guarantees that when the move is made
    // and unmade, it won't crash the program. It does a lot of general validations that should catch most corrupted moves.
    // It should be used to validate potentially corrupted moves, for example moves from TT.
    bool MoveGen::IsPseudoLegal(Move m) const {

        // ZERO_MOVE is always legal
        if (m == ZERO_MOVE) {
            return true;
        }

        // there exists a piece on the origin square and its of the correct color
        Square from = FromSquare(m);
        Piece moved_piece = board.GetPieceOnSquare(from);
        if (moved_piece == NO_PIECE || ColorOfPiece(moved_piece) != my_color) {
            return false;
        }

        // in double check - only king moves are allowed
        PieceType moved_pt = TypeOfPiece(moved_piece);
        if (double_check && moved_pt != KING) {
            return false;
        }

        // destination is either empty or occupied by enemy piece, but not a king (king captures are not allowed)
        Piece dest_piece = board.GetPieceOnSquare(ToSquare(m));
        if (dest_piece != NO_PIECE && (ColorOfPiece(dest_piece) == my_color || TypeOfPiece(dest_piece) == KING)) {
            return false;
        }

        // make sure we only move to the allowed squares
        Square to = ToSquare(m);
        if (!(SquareToBB(to) & legal_moves) && moved_pt != KING) {
            return false;
        }

        // castling validation, castling is a bit more complex because of chess960, we just generate the castling move
        // and compare them
        MoveType move_type = GetMoveType(m);
        if (move_type == CASTLING) {
            return my_color == WHITE ? ValidateCastling<WHITE>(m) : ValidateCastling<BLACK>(m);
        }

        // check that pawn move has the correct flag
        if (moved_pt == PAWN && move_type != NO_FLAG) {
            // has to have the correct move flag for a pawn, IsPromotion function isn't enough to validate promotions,
            // since that function is very lazy and only checks the promotion bit, and the flag itself still could be incorrect
            if (move_type != EN_PASSANT && move_type != TWO_FORWARD && move_type != PROMOTE_QUEEN &&
                move_type != PROMOTE_ROOK && move_type != PROMOTE_BISHOP && move_type != PROMOTE_KNIGHT) {
                return false;
            }

            // for double pawn push, make sure it is actually a double push and the destination is on the correct rank
            Bitboard mask = my_color == WHITE ? TwoFwdRank<WHITE>() : TwoFwdRank<BLACK>();
            if (move_type == TWO_FORWARD && ((to ^ from) != 16 || !(SquareToBB(to) & mask))) {
                return false;
            }

            // if en passant, make sure there is actually an ep square
            if (move_type == EN_PASSANT && ep_s != to) {
                return false;
            }
        }

        // for non-pawn moves, there's no flags that they can have now
        if (moved_pt != PAWN && move_type != NO_FLAG) {
            return false;
        }

        // our pseudo legal move gen doesn't allow discovery checks
        if (DiscoveryCheck(from, to)) {
            return false;
        }

        return true;
    }

    template<Color C>
    bool MoveGen::ValidateCastling(Move m) const {
        if (IsKingInCheck()) {
            return false;
        }
        if (CanCastle<C, SHORT>()) {
            if (m == NewMove(king_square, C == WHITE ? G1 : G8, CASTLING)) {
                return true;
            }
        }
        if (CanCastle<C, LONG>()) {
            if (m == NewMove(king_square, C == WHITE ? C1 : C8, CASTLING)) {
                return true;
            }
        }
        return false;
    }
}
