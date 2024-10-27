#include "MoveGen.h"

#include <ranges>

MoveGen::MoveGen(const Board &b, const Search::Killers &killer_moves) :
        board(b),
        my_color(board.ColorToMove()),
        enemy_color(OtherColor(my_color)),
        king_s(Bitboards::Lsb(board.Pieces(KING, my_color))),
        ep_s(board.EpSquare()),
        all_pieces(board.Pieces(ALL_TYPES)),
        empty_squares(~all_pieces),
        enemy_pieces(board.Pieces(enemy_color)),
        checkers(board.Checkers()),
        blockers(board.PinnedToSquare(king_s, enemy_color)),
        killers(killer_moves),
        killers_cnt(killer_moves.Size()) {

    if (checkers) {
        if (Bitboards::MoreThanOne(checkers)) {
            legal_moves = EMPTY_BB;
            gen_phase = DOUBLE_CHECK;
            double_check = true;
            return;
        }
        Bitboard capture_mask = checkers;
        Square attacker_square = Bitboards::Lsb(capture_mask);
        Bitboard block_mask = Bitboards::RayToSquares(king_s, attacker_square);
        legal_moves = capture_mask | block_mask;
    }
}

void MoveGen::EvalMoves() {
    for (auto &m_e: move_eval | std::views::take(moves_cnt)) {
        if (gen_phase == END && killers_cnt > 0) {
            if (auto k = killers.Find(m_e.move); k != killers.end()) {
                m_e.score = static_cast<Score>(std::distance(k, killers.end()) * KILLER_EVAL_BONUS);
                --killers_cnt;
                continue;
            }
        }
        m_e.score = board.MoveEval(m_e.move);
    }
}

template<MoveGen::GenType T>
Move MoveGen::NextBestMove() {
    while (Empty()) {
        my_color == WHITE ? NextPhase<WHITE, T>() : NextPhase<BLACK, T>();
        if (!Empty() && move_eval[0].move != ZERO_MOVE) {
            EvalMoves();
        }
    }
    auto it = std::max_element(move_eval.begin(), move_eval.begin() + moves_cnt);
    return PopRef(*it);
}

Move MoveGen::NextMove() {
    while (Empty()) {
        my_color == WHITE ? NextPhase<WHITE, NORMAL>() : NextPhase<BLACK, NORMAL>();
    }
    return PopBack();
}

template Move MoveGen::NextBestMove<MoveGen::QSEARCH>();
template Move MoveGen::NextBestMove<MoveGen::NORMAL>();

template<Color C, MoveGen::GenType T>
void MoveGen::NextPhase() {
    switch (gen_phase) {
        case DOUBLE_CHECK:
            GenMovesForPhase<DOUBLE_CHECK, C, T>();
            gen_phase = END;
            break;
        case PROMOTION:
            GenMovesForPhase<PROMOTION, C, T>();
            gen_phase = CAPTURE;
            break;
        case CAPTURE:
            GenMovesForPhase<CAPTURE, C, T>();
            gen_phase = T == QSEARCH && !checkers ? END : QUIET;
            break;
        case QUIET:
            GenMovesForPhase<QUIET, C, T>();
            gen_phase = END;
            break;
        case END:
            PutMove(ZERO_MOVE);
            break;
    }
}

template<MoveGen::GenPhase P, Color C, MoveGen::GenType T>
void MoveGen::GenMovesForPhase() {
    if constexpr (P == DOUBLE_CHECK) {
        MovesForPT<KING>(board.Pieces(KING, C), enemy_pieces | empty_squares);
    } else if constexpr (P == PROMOTION) {
        Bitboard pawns = board.Pieces(PAWN, C);
        PawnProms<C, LEFT, T>(pawns);
        PawnProms<C, RIGHT, T>(pawns);
        PawnProms<C, FORWARD, T>(pawns);
    } else if constexpr (P == CAPTURE || P == QUIET) {
        Bitboard pawns = board.Pieces(PAWN, C);
        if constexpr (P == CAPTURE) {
            EpMoves<C>(pawns);
            PawnCaptures<C, LEFT>(pawns);
            PawnCaptures<C, RIGHT>(pawns);
        } else {
            PawnOneFwd<C>(pawns);
            PawnTwoFwd<C>(pawns);
            CastlingMoves<C>();
        }
        Bitboard mask = P == CAPTURE ? legal_moves & enemy_pieces : legal_moves & empty_squares;
        MovesForPT<KNIGHT>(board.Pieces(KNIGHT, C), mask);
        MovesForPT<BISHOP>(board.Pieces(BISHOP, C), mask);
        MovesForPT<ROOK>(board.Pieces(ROOK, C), mask);
        MovesForPT<QUEEN>(board.Pieces(QUEEN, C), mask);
        MovesForPT<KING>(board.Pieces(KING, C), P == CAPTURE ? enemy_pieces : empty_squares);
    }
}

template<PieceType PT, MoveGen::GenMode M>
auto MoveGen::MovesForPT(Bitboard pieces, Bitboard legality_mask, Move to_validate) {
    while (pieces) {
        Square origin_s = Bitboards::PopLsb(pieces);
        Bitboard possible_moves = Bitboards::Attacks<PT>(origin_s, all_pieces) & legality_mask;
        if (blockers & SqToBB(origin_s)) {
            possible_moves &= Bitboards::RayToBorders(king_s, origin_s);
        }
        while (possible_moves) {
            Square dest_s = Bitboards::PopLsb(possible_moves);
            Move move = NewMove(origin_s, dest_s);
            if constexpr (M == GENERATE) PutMove(move);
            if constexpr (M == VALIDATE) if (move == to_validate) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::GenMode M>
auto MoveGen::PawnOneFwd(Bitboard pieces, Move to_validate) {

    constexpr Direction dir = PawnMove<C, FORWARD>();
    Bitboard one_fwd = Bitboards::Shift<dir>(pieces) & empty_squares & ~Bitboards::prom_mask[C] & legal_moves;

    while (one_fwd) {
        Square dest_s = Bitboards::PopLsb(one_fwd);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            Move pawn_one_fwd_move = NewMove(origin_s, dest_s);
            if constexpr (M == GENERATE) PutMove(pawn_one_fwd_move);
            if constexpr (M == VALIDATE) if (pawn_one_fwd_move == to_validate) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::GenMode M>
auto MoveGen::PawnTwoFwd(Bitboard pieces, Move to_validate) {

    constexpr Direction dir = PawnMove<C, FORWARD>();
    Bitboard two_fwd = Bitboards::Shift<dir>(Bitboards::Shift<dir>(pieces) & empty_squares) & empty_squares
                       & Bitboards::two_fwd_mask[C] & legal_moves;

    while (two_fwd) {
        Square dest_s = Bitboards::PopLsb(two_fwd);
        Square origin_s = dest_s - dir - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            Move pawn_two_fwd_move = NewMove(origin_s, dest_s, TWO_FORWARD);
            if constexpr (M == GENERATE) PutMove(pawn_two_fwd_move);
            if constexpr (M == VALIDATE) if (pawn_two_fwd_move == to_validate) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::PawnMoveDir D, MoveGen::GenMode M>
auto MoveGen::PawnCaptures(Bitboard pieces, Move to_validate) {

    constexpr Direction dir = PawnMove<C, D>();
    Bitboard captures = Bitboards::Shift<dir>(pieces) & legal_moves & enemy_pieces & ~Bitboards::prom_mask[C];

    while (captures) {
        Square dest_s = Bitboards::PopLsb(captures);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            Move pawn_capture_move = NewMove(origin_s, dest_s);
            if constexpr (M == GENERATE) PutMove(pawn_capture_move);
            if constexpr (M == VALIDATE) if (pawn_capture_move == to_validate) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::PawnMoveDir D, MoveGen::GenType T, MoveGen::GenMode M>
auto MoveGen::PawnProms(Bitboard pieces, Move to_validate) {

    constexpr Direction dir = PawnMove<C, D>();
    Bitboard promotions = Bitboards::Shift<dir>(pieces) & legal_moves & Bitboards::prom_mask[C];
    promotions &= D == FORWARD ? empty_squares : enemy_pieces;

    while (promotions) {
        Square dest_s = Bitboards::PopLsb(promotions);
        Square origin_s = dest_s - dir;
        if (!DiscoveryCheck(origin_s, dest_s)) {
            if constexpr (M == GENERATE) {
                if constexpr (T == QSEARCH) {
                    PutMove(NewMove(origin_s, dest_s, PROMOTE_QUEEN));
                } else {
                    PutPromMoves(NewMove(origin_s, dest_s));
                }
            }
            if constexpr (M == VALIDATE) if (ValidatePromMove(NewMove(origin_s, dest_s), to_validate)) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::GenMode M>
auto MoveGen::EpMoves(Bitboard pieces, Move to_validate) {
    if (ep_s) {
        Bitboard attackers = Bitboards::Attacks<PAWN>(ep_s, EMPTY_BB, OtherColor(C)) & pieces;
        while (attackers) {
            Square origin_s = Bitboards::PopLsb(attackers);
            Move ep_move = NewMove(origin_s, ep_s, EN_PASSANT);
            if constexpr (M == GENERATE) PutMove(ep_move);
            if constexpr (M == VALIDATE) if (ep_move == to_validate) return true;
        }
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, MoveGen::GenMode M>
auto MoveGen::CastlingMoves(Move to_validate) {
    if (checkers) {
        if constexpr (M == GENERATE) return;
        if constexpr (M == VALIDATE) return false;
    }
    if (CanCastle<C, SHORT>()) {
        Move shor_castling_move = NewMove(king_s, C == WHITE ? G1 : G8, CASTLING);
        if constexpr (M == GENERATE) PutMove(shor_castling_move);
        if constexpr (M == VALIDATE) if (shor_castling_move == to_validate) return true;
    }
    if (CanCastle<C, LONG>()) {
        Move long_castling_move = NewMove(king_s, C == WHITE ? C1 : C8, CASTLING);
        if constexpr (M == GENERATE) PutMove(long_castling_move);
        if constexpr (M == VALIDATE) if (long_castling_move == to_validate) return true;
    }
    if constexpr (M == VALIDATE) return false;
}

template<Color C, CastlingSide S>
bool MoveGen::CanCastle() const {
    if (!board.IsCastlingAvailable(C, S)) {
        return false;
    }
    // (for chess 960) we need to calculate all the squares that we travel through and make sure they are empty
    constexpr Square r_dest = S == LONG ? C == WHITE ? D1 : D8 : C == WHITE ? F1 : F8;
    constexpr Square k_dest = S == LONG ? C == WHITE ? C1 : C8 : C == WHITE ? G1 : G8;
    Bitboard rook_bb = board.RookSqBB(C, S);
    Bitboard occ = all_pieces ^ rook_bb ^ SqToBB(king_s);
    Bitboard walk_sq = Bitboards::RayToSquares(Bitboards::Lsb(rook_bb), r_dest)
                       | Bitboards::RayToSquares(king_s, k_dest)
                       | SqToBB(r_dest) | SqToBB(k_dest);

    return (occ & walk_sq) == EMPTY_BB;
}

// allow movement only on a line between piece and king, if piece is a blocker
bool MoveGen::DiscoveryCheck(Square origin, Square destination) const {
    return (blockers & SqToBB(origin)) && !(Bitboards::RayToBorders(king_s, origin) & SqToBB(destination));
}

// guarantees that the move can be generated by this generator
bool MoveGen::IsPseudoLegal(Move m) {

    Piece moved_piece = board.PieceOnSquare(FromSquare(m));
    if (moved_piece == NO_PIECE || ColorOfPiece(moved_piece) != my_color) {
        return false;
    }

    if (double_check) {
        if (TypeOfPiece(moved_piece) != KING) return false;
        return MovesForPT<KING, VALIDATE>(SqToBB(FromSquare(m)), empty_squares | enemy_pieces, m);
    }

    if (TypeOfMove(m) == NO_FLAG) {
        return my_color == WHITE ? NormalMoveIsPseudoLegal<WHITE>(m) : NormalMoveIsPseudoLegal<BLACK>(m);
    } else {
        return my_color == WHITE ? SpecialMoveIsPseudoLegal<WHITE>(m) : SpecialMoveIsPseudoLegal<BLACK>(m);
    }
}

template<Color C>
bool MoveGen::SpecialMoveIsPseudoLegal(Move m) {

    MoveType mt = TypeOfMove(m);
    Square from = FromSquare(m);
    PieceType moved_pt = board.PieceTypeOnSq(from);
    Bitboard pos = SqToBB(from);

    if (mt == TWO_FORWARD && moved_pt == PAWN) {
        return PawnTwoFwd<C, VALIDATE>(pos, m);
    } else if (mt == CASTLING && moved_pt == KING) {
        return CastlingMoves<C, VALIDATE>(m);
    } else if (mt == EN_PASSANT && moved_pt == PAWN) {
        return EpMoves<C, VALIDATE>(pos, m);
    } else if (IsPromotion(m) && moved_pt == PAWN) {
        return PawnProms<C, LEFT, NORMAL, VALIDATE>(pos, m)
               || PawnProms<C, RIGHT, NORMAL, VALIDATE>(pos, m)
               || PawnProms<C, FORWARD, NORMAL, VALIDATE>(pos, m);
    }
    return false;
}

template<Color C>
bool MoveGen::NormalMoveIsPseudoLegal(Move m) {

    Square from = FromSquare(m);
    PieceType moved_pt = board.PieceTypeOnSq(from);
    Bitboard pos = SqToBB(from);

    if (moved_pt == PAWN) {
        if (board.PieceOnSquare(ToSquare(m)) == NO_PIECE) {
            return PawnOneFwd<C, VALIDATE>(pos, m);
        } else {
            return PawnCaptures<C, LEFT, VALIDATE>(pos, m) || PawnCaptures<C, RIGHT, VALIDATE>(pos, m);
        }
    }

    Bitboard mask = (empty_squares | enemy_pieces) & legal_moves;
    return moved_pt == KNIGHT ? MovesForPT<KNIGHT, VALIDATE>(pos, mask, m) :
           moved_pt == BISHOP ? MovesForPT<BISHOP, VALIDATE>(pos, mask, m) :
           moved_pt == ROOK ? MovesForPT<ROOK, VALIDATE>(pos, mask, m) :
           moved_pt == QUEEN ? MovesForPT<QUEEN, VALIDATE>(pos, mask, m) :
           moved_pt == KING && MovesForPT<KING, VALIDATE>(pos, empty_squares | enemy_pieces, m);
}

bool MoveGen::ValidatePromMove(Move m, Move to_validate) {
    return ((m | PROMOTE_QUEEN) == to_validate)
           || ((m | PROMOTE_ROOK) == to_validate)
           || ((m | PROMOTE_BISHOP) == to_validate)
           || ((m | PROMOTE_KNIGHT) == to_validate);
}
