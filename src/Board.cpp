#include "Board.h"
#include "Bitboards.h"
#include "FenLoader.h"
#include "Misc.h"
#include <cstring>

namespace Meetra {

    // TODO move the initialization from fen to SetPosition function, and make it more efficient (directly read into the
    // bitboards, arrays, game state and such so we dont have to copy everything, this takes forever
    // make LoadFen function that takes in game state and other arrays as arguments and fills them

    Board::Board(std::string fen) {
        history_cnt = 0;
        game_state = NEW_GAME_STATE;
        auto loadedInfo = Meetra::FenLoader::ParseFen(std::move(fen));
        SetColorToMove(loadedInfo->color_to_move);
        if (loadedInfo->w_castle_short) { SetCastlingRights(WHITE_SHORT); }
        if (loadedInfo->w_castle_long) { SetCastlingRights(WHITE_LONG); }
        if (loadedInfo->b_castle_short) { SetCastlingRights(BLACK_SHORT); }
        if (loadedInfo->b_castle_long) { SetCastlingRights(BLACK_LONG); }
        SetEpSquare(loadedInfo->ep_square);
        SetPly(loadedInfo->ply);
        SetMoveNumber(loadedInfo->full_move_count);
        std::memcpy(board, loadedInfo->board_occ, sizeof(Piece) * SQUARE_NR);

        for (Square s = A1; s <= H8; ++s) {
            Piece p = board[s];
            if (p != NO_PIECE) {
                Bitboard pos = SquareToBB(s);
                color_bbs[ColorOfPiece(p)] |= pos;
                type_bbs[TypeOfPiece(p)] |= pos;
                type_bbs[ALL_TYPES] |= pos;
            }
        }
    }

    Bitboard Board::PinnedPiecesForSquare(Square s, Color attackers_color, Bitboard &pinning_pieces) const {

        Bitboard pinned_pieces = EMPTY_BB;
        Bitboard potential_blockers = GetPieces(ALL_TYPES);

        Bitboard bishop_queen_attackers = GetPieces(BISHOP, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (bishop_queen_attackers) {
            Square attacker_s = PopLsb(bishop_queen_attackers);
            Bitboard blockers = rays_between_squares[attacker_s][s] & potential_blockers & bishop_moves[attacker_s];
            if (blockers && !MoreThanOne(blockers)) {
                pinned_pieces |= blockers;
                pinning_pieces |= SquareToBB(attacker_s);
            }
        }

        Bitboard rook_queen_attackers = GetPieces(ROOK, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (rook_queen_attackers) {
            Square attacker_s = PopLsb(rook_queen_attackers);
            Bitboard blockers = rays_between_squares[attacker_s][s] & potential_blockers & rook_moves[attacker_s];
            if (blockers && !MoreThanOne(blockers)) {
                pinned_pieces |= blockers;
                pinning_pieces |= SquareToBB(attacker_s);
            }
        }

        return pinned_pieces;
    }

    bool Board::IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const {
        return GetAttacksForPiece<ROOK>(s, occ) & (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
               GetAttacksForPiece<BISHOP>(s, occ) & (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
               GetAttacksForPiece<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by) ||
               GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by) ||
               GetAttacksForPiece<KING>(s) & GetPieces(KING, attacked_by);
    }

    Bitboard Board::SquareAttackers(Square s, Color attacked_by, Bitboard occ) const {
        return (GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by)) |
               (GetAttacksForPiece<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by)) |
               (GetAttacksForPiece<BISHOP>(s, occ) &
                (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<ROOK>(s, occ) &
                (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<KING>(s) & GetPieces(KING, attacked_by));
    }

    bool Board::MakeMove(Move m) {

        history[history_cnt++] = game_state;

        Color this_move_col = ColorToMove();
        ChangeColorToMove();
        Color next_move_col = ColorToMove();

        ClearCapturedPiece();
        ClearEpSquare();
        IncrementPly();

        IncrementMoveNumber(this_move_col);

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        RemoveCastlingRights(static_cast<CastlingRights>(castling_mask[from] | castling_mask[to]));

        MoveType move_type = GetFlag(m);
        Square capture_square = to;
        Piece captured_piece = board[to];
        Piece moved_piece = board[from];
        PieceType moved_piece_type = TypeOfPiece(moved_piece);

        if (move_type == EN_PASSANT) {
            capture_square += next_move_col ? SOUTH : NORTH;
            captured_piece = NewPiece(PAWN, next_move_col);
        }

        if (captured_piece != NO_PIECE) {
            RemovePiece(capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        } else if (moved_piece_type != PAWN) {
            ResetPly();
        }

        MovePiece(from, to);

        if (move_type) {
            if (move_type == EN_PASSANT) {
                return !IsSquareAttacked(Lsb(GetPieces(KING, this_move_col)), next_move_col, GetPieces(ALL_TYPES));
            } else if (move_type == TWO_FORWARD) {
                SetEpSquare(next_move_col ? to + SOUTH : to + NORTH);
            } else if (move_type == CASTLING) {
                Square castling_rook_orig = RookFromCastling(to);
                Square castling_rook_dest = RookToCastling(to);
                MovePiece(castling_rook_orig, castling_rook_dest);
            } else if (IsPromotion(m)) {
                RemovePiece(to);
                PutPiece(to, NewPiece(PieceTypeFromFlag(move_type), this_move_col));
            }
        }

        if (moved_piece_type == KING) {
            return !IsSquareAttacked(Lsb(GetPieces(KING, this_move_col)), next_move_col, GetPieces(ALL_TYPES));
        }
        return true;
    }

    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);
        MoveType move_type = GetFlag(m);

        Color black_to_move = ColorToMove();
        Piece captured_piece = CapturedPiece();

        if (IsPromotion(m)) {
            RemovePiece(to);
            PutPiece(to, NewPiece(PAWN, static_cast<Color>(!black_to_move)));
        }

        MovePiece(to, from);

        if (move_type == EN_PASSANT) {
            PutPiece(black_to_move ? to + SOUTH : to + NORTH, captured_piece);
        } else if (captured_piece) {
            PutPiece(to, captured_piece);
        } else if (move_type == CASTLING) {
            Square castling_rook_orig = RookFromCastling(to);
            Square castling_rook_dest = RookToCastling(to);
            MovePiece(castling_rook_dest, castling_rook_orig);
        }

        game_state = history[--history_cnt];
    }

    std::string Board::PPBoard() const {
        std::string ret;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            ret.append(std::to_string(r + 1));
            ret.append(" |");
            for (File f = FILE_A; f <= FILE_H; ++f) {
                ret.push_back(' ');
                ret.push_back(PieceToChar(board[SquareFromFiRa(f, r)]));
                ret.push_back(' ');
            }
            ret.append("\n");
        }
        ret.append("---------------------------\n");
        ret.append("  | A  B  C  D  E  F  G  H\n\n");
        ret.append("Player to move: ");
        ColorToMove() == WHITE ? ret.append("white\n") : ret.append("black\n");
        ret.append("Move count: ");
        ret.append(std::to_string(TotalMoves()));
        ret.append(" | Ply since last capture: ");
        ret.append(std::to_string(Ply()));
        ret.append("\nCastling rights: ");
        bool castling_available = false;
        if (CanWhiteShortCR()) {
            ret.push_back('K');
            castling_available = true;
        }
        if (CanWhiteLongCR()) {
            ret.push_back('Q');
            castling_available = true;
        }
        if (CanBlackShortCR()) {
            ret.push_back('k');
            castling_available = true;
        }
        if (CanBlackLongCR()) {
            ret.push_back('q');
            castling_available = true;
        }
        if (!castling_available) { ret.push_back('-'); }
        ret.append(" | EP square: ");
        if (EpSquare() != SQUARE_ZERO) { ret.append(std::to_string(EpSquare())); }
        else { ret.push_back('-'); }
        return ret;
    }


}
