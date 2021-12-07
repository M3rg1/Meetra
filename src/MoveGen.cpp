#include "MoveGen.h"

template<Color C, PawnMoveDir DIR>
constexpr Direction PawnMove() {
    return C == WHITE ? DIR == LEFT ? NORTH_WEST : DIR == RIGHT ? NORTH_EAST : NORTH
                      : DIR == LEFT ? SOUTH_EAST : DIR == RIGHT ? SOUTH_WEST : SOUTH;
}

template<PawnMoveDir DIR>
constexpr Direction PawnMove(Color c) {
    return c == WHITE ? DIR == LEFT ? NORTH_WEST : DIR == RIGHT ? NORTH_EAST : NORTH
                      : DIR == LEFT ? SOUTH_EAST : DIR == RIGHT ? SOUTH_WEST : SOUTH;
}

MoveGen::MoveGen(const Board &board, const Move killer_moves[]) : MoveGen(board) {
    std::copy_n(killer_moves, KILLER_SLOTS, killers);
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
        legal_moves(FULL_BB),
        double_check(false),
        gen_phase(PROMOTION),
        killers{},
        moves_cnt(0) {

    if (checkers) {
        if (Bitboards::MoreThanOne(checkers)) {
            legal_moves = EMPTY_BB;
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
    for (size_t i = 0, j = 0; i < moves_cnt; ++i, j = 0) {
        for (; j < KILLER_SLOTS; ++j) {
            if (move_eval[i].move == killers[j]) {
                move_eval[i].score = static_cast<Score>((KILLER_SLOTS - j) * 10000);
                break;
            }
        }
        if (j >= KILLER_SLOTS) {
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
    Bitboard one_fwd = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & empty_squares & ~Bitboards::prom_mask[C];
    Bitboard two_fwd = Bitboards::Shift<dir>(one_fwd) & empty_squares & Bitboards::two_fwd_mask[C] & legal_moves;
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
    Bitboard captures = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & legal_moves & enemy_pieces
                        & ~Bitboards::prom_mask[C];

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
    Bitboard promotions = Bitboards::Shift<dir>(board.GetPieces(PAWN, C)) & legal_moves & Bitboards::prom_mask[C];
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

    if (!board.CrAvailable(C, S)) {
        return false;
    }

    // (for chess 960) we need to calculate all the squares that we travel through and make sure they are empty
    constexpr Square r_dest = S == LONG ? C == WHITE ? D1 : D8 : C == WHITE ? F1 : F8;
    constexpr Square k_dest = S == LONG ? C == WHITE ? C1 : C8 : C == WHITE ? G1 : G8;
    Bitboard rook_bb = board.RookSqBB(C, S);
    Bitboard occ = all_pieces ^ rook_bb ^ SqToBB(king_s);
    Bitboard walk_sq = Bitboards::GetRayToSquares(Bitboards::Lsb(rook_bb), r_dest)
                       | Bitboards::GetRayToSquares(king_s, k_dest)
                       | SqToBB(r_dest) | SqToBB(k_dest);

    return (occ & walk_sq) == EMPTY_BB;
}

// this function does not guarantee the move is actually pseudo legal, it only guarantees that when the move is made
// and unmade, it won't crash the program. It does a lot of general validations that should catch most corrupted moves.
bool MoveGen::IsPseudoLegal(Move m) const {

    if (m == ZERO_MOVE) {
        return false;
    }

    MoveType move_type = GetMoveType(m);

    if (move_type != NO_FLAG) {
        if (move_type == TWO_FORWARD) {
            return my_color == WHITE ? ValidateTwoFwd<WHITE>(m) : ValidateTwoFwd<BLACK>(m);
        } else if (move_type == CASTLING) {
            return my_color == WHITE ? ValidateCastling<WHITE>(m) : ValidateCastling<BLACK>(m);
        } else if (move_type == EN_PASSANT) {
            return my_color == WHITE ? ValidateEp<WHITE>(m) : ValidateEp<BLACK>(m);
        } else if (IsPromotion(m)) {
            return my_color == WHITE ? ValidateProm<WHITE>(m) : ValidateProm<BLACK>(m);
        }
        return false;
    }

    Square from = FromSquare(m);
    Piece moved_piece = board.GetPieceOnSquare(from);
    // there exists a piece on the origin square and its of the correct color
    if (moved_piece == NO_PIECE || ColorOfPiece(moved_piece) != my_color) {
        return false;
    }

    PieceType moved_pt = TypeOfPiece(moved_piece);
    // in double check - only king moves are allowed
    if (double_check && moved_pt != KING) {
        return false;
    }

    Square to = ToSquare(m);
    Piece captured_piece = board.GetPieceOnSquare(to);
    // destination is either empty or occupied by enemy piece, but not a king
    if (captured_piece != NO_PIECE
        && (ColorOfPiece(captured_piece) == my_color || TypeOfPiece(captured_piece) == KING)) {
        return false;
    }

    if (moved_pt == KING) {
        if (SqToBB(to) & (enemy_pieces | empty_squares)) {
            return true;
        }
        return false;
    }

    if (!(SqToBB(to) & legal_moves) || DiscoveryCheck(from, to)) {
        return false;
    }

    if (moved_pt == PAWN) {

        if (SqToBB(to) & (Bitboards::prom_mask[my_color] | Bitboards::prom_mask[OtherColor(my_color)])) {
            return false;
        }

        if (captured_piece == NO_PIECE) {
            if (Bitboards::Shift(SqToBB(from), PawnMove<FORWARD>(my_color)) != SqToBB(to)) {
                return false;
            }
        } else {
            if (Bitboards::Shift(SqToBB(from), PawnMove<LEFT>(my_color)) != SqToBB(to)
                && Bitboards::Shift(SqToBB(from), PawnMove<RIGHT>(my_color)) != SqToBB(to)) {
                return false;
            }
        }
    }

    return true;
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

template<Color C>
bool MoveGen::ValidateEp(Move m) const {
    if (ep_s) {
        Bitboard attackers = Bitboards::GetAttacks<PAWN>(ep_s, EMPTY_BB, OtherColor(C)) & board.GetPieces(PAWN, C) & SqToBB(FromSquare(m));;
        while (attackers) {
            Square from = Bitboards::PopLsb(attackers);
            if (NewMove(from, ep_s, EN_PASSANT) == m) {
                return true;
            }
        }
    }
    return false;
}

template<Color C>
bool MoveGen::ValidateProm(Move m) const {
    if (ValidatePromHelper<C, FORWARD>(m) || ValidatePromHelper<C, LEFT>(m) || ValidatePromHelper<C, RIGHT>(m)) {
        return true;
    }
    return false;
}

template<Color C, PawnMoveDir D>
bool MoveGen::ValidatePromHelper(Move m) const {

    MoveType mt = GetMoveType(m);
    if (mt != PROMOTE_QUEEN && mt != PROMOTE_ROOK && mt != PROMOTE_BISHOP && mt != PROMOTE_KNIGHT) {
        return false;
    }

    m &= 0xFFF;
    constexpr Direction dir = PawnMove<C, D>();
    Bitboard pos = board.GetPieces(PAWN, C) & SqToBB(FromSquare(m));
    Bitboard promotions = Bitboards::Shift<dir>(pos) & legal_moves & Bitboards::prom_mask[C];
    promotions &= D == FORWARD ? empty_squares : enemy_pieces;

    while (promotions) {
        Square dest_s = Bitboards::PopLsb(promotions);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            if (NewMove(origin_s, dest_s) == m) {
                return true;
            }
        }
    }
    return false;
}

template<Color C>
bool MoveGen::ValidateTwoFwd(Move m) const {

    constexpr Direction dir = PawnMove<C, FORWARD>();
    Bitboard pos = board.GetPieces(PAWN, C) & SqToBB(FromSquare(m));
    Bitboard one_fwd = Bitboards::Shift<dir>(pos) & empty_squares & ~Bitboards::prom_mask[C];
    Bitboard two_fwd = Bitboards::Shift<dir>(one_fwd) & empty_squares & Bitboards::two_fwd_mask[C] & legal_moves;

    while (two_fwd) {
        Square dest_s = Bitboards::PopLsb(two_fwd);
        Square origin_s = dest_s - dir - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            if (NewMove(origin_s, dest_s, TWO_FORWARD) == m) {
                return true;
            }
        }
    }
    return false;
}
