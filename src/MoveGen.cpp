#include "MoveGen.h"

template<Color C, PawnMoveDir DIR>
constexpr Direction PawnMove() {
    return C == WHITE ? DIR == LEFT ? NORTH_WEST : DIR == RIGHT ? NORTH_EAST : NORTH :
           DIR == LEFT ? SOUTH_EAST : DIR == RIGHT ? SOUTH_WEST : SOUTH;
}

template<Color C>
constexpr Bitboard PromRank() {
    return C == WHITE ? 0xFF00000000000000 : 0xFF000000000000FF;
}

template<Color C>
constexpr Bitboard TwoFwdRank() {
    return C == WHITE ? 0x00000000FF000000 : 0x000000FF00000000;
}

MoveGen::MoveGen(const Board &board, const Move killer_moves[2]) : MoveGen(board) {
    killers[0] = killer_moves[0];
    killers[1] = killer_moves[1];
}

MoveGen::MoveGen(const Board &board) :
        board(board),
        my_color(board.ColorToMove()),
        enemy_color(OtherColor(my_color)),
        king_s(Bitboards::Lsb(board.GetPieces(KING, my_color))),
        ep_s(board.EpSquare()),
        all_pieces(board.GetPieces_pt(ALL_TYPES)),
        empty_squares(~all_pieces),
        enemy_pieces(board.GetPieces_c(enemy_color)),
        checkers(board.GetCheckers()),
        blockers(board.PinnedToSquare(king_s, enemy_color)),
        legal_moves(0xFFFFFFFFFFFFFFFF),
        double_check(false),
        gen_phase(PROMOTION),
        killers{ZERO_MOVE, ZERO_MOVE},
        moves_cnt(0) {

    if (checkers) {
        if (Bitboards::MoreThanOne(checkers)) {
            legal_moves = 0;
            gen_phase = DOUBLE_CHECK;
            double_check = true;
            return;
        }
        Bitboard capture_mask = checkers;
        Square attacker_square = Bitboards::Lsb(capture_mask);
        Bitboard block_mask = Bitboards::GetRayToSquares(king_s, attacker_square);
        legal_moves = capture_mask | block_mask;
    }
}

void MoveGen::EvalMoves() {
    for (int i = 0; i < moves_cnt; ++i) {
        if (move_eval[i].move == killers[0]) {
            move_eval[i].score = 90000;
        } else if (move_eval[i].move == killers[1]) {
            move_eval[i].score = 80000;
        } else {
            move_eval[i].score = board.GetMoveEval(move_eval[i].move);
        }
    }
}

template<GenType T>
Move MoveGen::GetBestMove() {
    while (Empty()) {
        my_color == WHITE ? NextPhase<WHITE, T>() : NextPhase<BLACK, T>();
        if (!Empty() && move_eval[0].move != ZERO_MOVE) {
            EvalMoves();
        }
    }
    auto it = std::max_element(move_eval, move_eval + moves_cnt);
    return PopRef(*it);
}

Move MoveGen::GetAnyMove() {
    while (Empty()) {
        my_color == WHITE ? NextPhase<WHITE, NORMAL>() : NextPhase<BLACK, NORMAL>();
    }
    return PopBack();
}

template Move MoveGen::GetBestMove<QSEARCH>();
template Move MoveGen::GetBestMove<NORMAL>();

template<Color C, GenType T>
void MoveGen::NextPhase() {
    switch (gen_phase) {
        case DOUBLE_CHECK:
            GenMovesForPhase<DOUBLE_CHECK, C>();
            gen_phase = END;
            break;
        case PROMOTION:
            GenMovesForPhase<PROMOTION, C>();
            gen_phase = CAPTURE;
            break;
        case CAPTURE:
            GenMovesForPhase<CAPTURE, C>();
            gen_phase = T == QSEARCH && !checkers ? END : QUIET;
            break;
        case QUIET:
            GenMovesForPhase<QUIET, C>();
            gen_phase = END;
            break;
        case END:
            PutMove(ZERO_MOVE);
            break;
    }
}

template<GenPhase P, Color C>
void MoveGen::GenMovesForPhase() {
    if constexpr (P == DOUBLE_CHECK) {
        GenMovesForPieceType<KING, C>(enemy_pieces | empty_squares);
    } else if constexpr (P == PROMOTION) {
        GenPawnPromotions<C, LEFT>();
        GenPawnPromotions<C, RIGHT>();
        GenPawnPromotions<C, FORWARD>();
    } else if constexpr (P == CAPTURE) {
        GenPawnCaptures<C, LEFT>();
        GenPawnCaptures<C, RIGHT>();
        GenEpMoves<C>();
        GenMovesForPieceType<KING, C>(enemy_pieces);
        GenMovesForPieceType<KNIGHT, C>(legal_moves & enemy_pieces);
        GenMovesForPieceType<BISHOP, C>(legal_moves & enemy_pieces);
        GenMovesForPieceType<ROOK, C>(legal_moves & enemy_pieces);
        GenMovesForPieceType<QUEEN, C>(legal_moves & enemy_pieces);
    } else if constexpr (P == QUIET) {
        GenCastlingMoves<C>();
        GenPawnQuiets<C>();
        GenMovesForPieceType<KING, C>(empty_squares);
        GenMovesForPieceType<KNIGHT, C>(legal_moves & empty_squares);
        GenMovesForPieceType<BISHOP, C>(legal_moves & empty_squares);
        GenMovesForPieceType<ROOK, C>(legal_moves & empty_squares);
        GenMovesForPieceType<QUEEN, C>(legal_moves & empty_squares);
    }
}

template<PieceType PT, Color C>
void MoveGen::GenMovesForPieceType(Bitboard legality_mask) {
    Bitboard pieces = board.GetPieces(PT, C);
    while (pieces) {
        Square origin_s = Bitboards::PopLsb(pieces);
        Bitboard possible_moves = Bitboards::GetAttacks<PT>(origin_s, all_pieces) & legality_mask;
        if (blockers & SqToBB(origin_s)) {
            possible_moves &= Bitboards::GetRayToBorders(king_s, origin_s);
        }
        while (possible_moves) {
            Square destination_s = Bitboards::PopLsb(possible_moves);
            PutMove(NewMove(origin_s, destination_s));
        }
    }
}

template<Color C>
void MoveGen::GenPawnQuiets() {

    constexpr Direction dir = PawnMove<C, FORWARD>();
    Bitboard one_fwd = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & empty_squares & ~PromRank<C>();
    Bitboard two_fwd = Bitboards::Shift<dir>(one_fwd) & empty_squares & TwoFwdRank<C>() & legal_moves;
    one_fwd &= legal_moves;

    while (two_fwd) {
        Square dest_s = Bitboards::PopLsb(two_fwd);
        Square origin_s = dest_s - dir - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            PutMove(NewMove(origin_s, dest_s, TWO_FORWARD));
        }
    }

    while (one_fwd) {
        Square dest_s = Bitboards::PopLsb(one_fwd);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            PutMove(NewMove(origin_s, dest_s));
        }
    }
}

template<Color C, PawnMoveDir D>
void MoveGen::GenPawnCaptures() {

    constexpr Direction dir = PawnMove<C, D>();
    Bitboard captures = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & legal_moves & enemy_pieces & ~PromRank<C>();

    while (captures) {
        Square dest_s = Bitboards::PopLsb(captures);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            PutMove(NewMove(origin_s, dest_s));
        }
    }
}

template<Color C, PawnMoveDir D>
void MoveGen::GenPawnPromotions() {

    constexpr Direction dir = PawnMove<C, D>();
    Bitboard promotions = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & legal_moves & PromRank<C>();
    promotions &= D == FORWARD ? empty_squares : enemy_pieces;

    while (promotions) {
        Square dest_s = Bitboards::PopLsb(promotions);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            PutPromMoves(NewMove(origin_s, dest_s));
        }
    }
}

// allow movement only on a line between piece and king, if piece is a blocker
bool MoveGen::DiscoveryCheck(Square orig, Square dest) const {
    return (blockers & SqToBB(orig)) && !(Bitboards::GetRayToBorders(king_s, orig) & SqToBB(dest));
}

template<Color C>
void MoveGen::GenEpMoves() {
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
    if (checkers) {
        return;
    }
    if (CanCastle<C, SHORT>()) {
        PutMove(NewMove(king_s, C == WHITE ? G1 : G8, CASTLING));
    }
    if (CanCastle<C, LONG>()) {
        PutMove(NewMove(king_s, C == WHITE ? C1 : C8, CASTLING));
    }
}

template<Color C, CastlingSide S>
bool MoveGen::CanCastle() const {

    Bitboard rook_bb = board.RookSqBB(C, S);
    if (!rook_bb) {
        return false;
    }

    // (for chess 960) we need to calculate all the squares that we travel through and make sure they are empty
    constexpr Square r_dest = S == LONG ? C == WHITE ? D1 : D8 : C == WHITE ? F1 : F8;
    constexpr Square k_dest = S == LONG ? C == WHITE ? C1 : C8 : C == WHITE ? G1 : G8;
    Bitboard occ = all_pieces ^ rook_bb ^ SqToBB(king_s);
    Bitboard walk_sq = Bitboards::GetRayToSquares(Bitboards::Lsb(rook_bb), r_dest) |
                       Bitboards::GetRayToSquares(king_s, k_dest) |
                       SqToBB(r_dest) | SqToBB(k_dest);

    return (occ & walk_sq) == EMPTY_BB;
}

// this function does not guarantee the move is actually pseudo legal, it only guarantees that when the move is made
// and unmade, it won't crash the program. It does a lot of general validations that should catch most corrupted moves.
bool MoveGen::IsPseudoLegal(Move m) const {

    if (m == ZERO_MOVE) {
        return false;
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

    // castling is a bit more complex because of chess960 - just generate the castling move and compare them
    MoveType move_type = GetMoveType(m);
    if (move_type == CASTLING) {
        return my_color == WHITE ? ValidateCastling<WHITE>(m) : ValidateCastling<BLACK>(m);
    }

    // destination is either empty or occupied by enemy piece, but not a king (king captures can crash the engine)
    Square to = ToSquare(m);
    Piece dest_piece = board.GetPieceOnSquare(to);
    if (dest_piece != NO_PIECE && (ColorOfPiece(dest_piece) == my_color || TypeOfPiece(dest_piece) == KING)) {
        return false;
    }

    // make sure we only move to the allowed squares
    if (moved_pt != KING && moved_pt != PAWN && !(SqToBB(to) & legal_moves)) {
        return false;
    }

    // special checks for pawn moves
    if (moved_pt == PAWN) {

        Bitboard prom_mask = my_color == WHITE ? PromRank<WHITE>() : PromRank<BLACK>();
        if (move_type == PROMOTE_QUEEN || move_type == PROMOTE_ROOK || move_type == PROMOTE_BISHOP
            || move_type == PROMOTE_KNIGHT) {
            if (!(prom_mask & SqToBB(to)) || DiscoveryCheck(from, to)) {
                return false;
            }
            return true;
        }

        if (prom_mask & SqToBB(to)) {
            return false;
        }

        if (move_type == EN_PASSANT) {
            if (ep_s != to) {
                return false;
            }
            return true;
        }

        if (move_type == TWO_FORWARD) {
            Square between = from + (my_color == WHITE ? NORTH : SOUTH);
            Bitboard mask = my_color == WHITE ? TwoFwdRank<WHITE>() : TwoFwdRank<BLACK>();
            if ((to ^ from) != 16 || !(SqToBB(to) & mask) || (SqToBB(between) & all_pieces)
                || DiscoveryCheck(from, to)) {
                return false;
            }
            return true;
        }
    }

    return move_type == NO_FLAG;
}

template<Color C>
bool MoveGen::ValidateCastling(Move m) const {
    if (checkers) {
        return false;
    }
    if (CanCastle<C, SHORT>()) {
        if (m == NewMove(king_s, C == WHITE ? G1 : G8, CASTLING)) {
            return true;
        }
    }
    if (CanCastle<C, LONG>()) {
        if (m == NewMove(king_s, C == WHITE ? C1 : C8, CASTLING)) {
            return true;
        }
    }
    return false;
}
