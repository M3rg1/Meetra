#include "Board.h"
#include <utility>
#include "Bitboards.h"
#include "FenLoader.h"
#include "Misc.h"
#include <cstring>
#include <chrono>

namespace Meetra {

    // TODO move the initialization from fen to SetPosition function, and make it more efficient (directly read into the
    // bitboards, arrays, game state and such so we dont have to copy everything, this takes forever
    // make LoadFen function that takes in game state and other arrays as arguments and fills them

    Board::Board(std::string fen) {
        gs_history = std::deque<BoardData>(100);
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

        Square king_square = Lsb(GetPieces(KING, ColorToMove()));
        checkers = SquareAttackers(king_square, ColorToMove() == WHITE ? BLACK : WHITE, GetPieces(ALL_TYPES));
    }


    bool Board::MakeMove(Move m) {

        gs_history.emplace_back(BoardData{game_state, checkers});

        ChangeColorToMove();
        ClearCapturedPiece();
        ClearEpSquare();
        IncrementPly();

        Color black_moving = ColorToMove();
        IncrementMoveNumber(!black_moving);

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        RemoveCastlingRights(static_cast<CastlingRights>(castling_mask[from] | castling_mask[to]));

        MoveType move_type = GetFlag(m);
        Square capture_square = to;
        Piece captured_piece = board[to];
        Piece moved_piece = board[from];

        if (move_type == EN_PASSANT) {
            capture_square += black_moving ? SOUTH : NORTH;
            captured_piece = NewPiece(PAWN, black_moving);
        }

        // TODO test if ply and move counter is incrementing/resetting correctly

        if (captured_piece != NO_PIECE) {
            RemovePiece(capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        } else if (TypeOfPiece(moved_piece) != PAWN) {
            ResetPly();
        }

        MovePiece(from, to);

        if (move_type != NO_FLAG) {
            if (move_type == TWO_FORWARD) {
                SetEpSquare(black_moving ? to + SOUTH : to + NORTH);
            } else if (move_type == CASTLING) {
                Square castling_rook_orig = RookFromCastling(to);
                Square castling_rook_dest = RookToCastling(to);
                MovePiece(castling_rook_orig, castling_rook_dest);
            } else if (IsPromotion(m)) {
                RemovePiece(to);
                switch (move_type) {
                    case PROMOTE_QUEEN:
                        PutPiece(to, NewPiece(QUEEN, static_cast<Color>(!black_moving)));
                        break;
                    case PROMOTE_ROOK:
                        PutPiece(to, NewPiece(ROOK, static_cast<Color>(!black_moving)));
                        break;
                    case PROMOTE_BISHOP:
                        PutPiece(to, NewPiece(BISHOP, static_cast<Color>(!black_moving)));
                        break;
                    case PROMOTE_KNIGHT:
                        PutPiece(to, NewPiece(KNIGHT, static_cast<Color>(!black_moving)));
                        break;
                    default:
                        break;
                }
            }
        }

        Square king_square = Lsb(GetPieces(KING, static_cast<Color>(!black_moving)));
        if (SquareAttackers(king_square, static_cast<Color>(black_moving), GetPieces(ALL_TYPES))) {
            return false;
        }

        king_square = Lsb(GetPieces(KING, static_cast<Color>(black_moving)));
        checkers = SquareAttackers(king_square, static_cast<Color>(!black_moving), GetPieces(ALL_TYPES));

        return true;
    }


    Bitboard Board::SquareAttackers(Square s, Color attacked_by, Bitboard occ) const {
        return (GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by)) |
               (GetAttacksForPiece<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by)) |
               (GetAttacksForPiece<BISHOP>(s, occ) & GetPieces(BISHOP, attacked_by)) |
               (GetAttacksForPiece<ROOK>(s, occ) & GetPieces(ROOK, attacked_by)) |
               (GetAttacksForPiece<QUEEN>(s, occ) & GetPieces(QUEEN, attacked_by)) |
               (GetAttacksForPiece<KING>(s) & GetPieces(KING, attacked_by));
    }

    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);
        MoveType move_type = GetFlag(m);

        Color black_to_move = ColorToMove();
        Piece captured_piece = CapturedPiece();

        if (IsPromotion(m)) {
            RemovePiece(to);
            PutPiece(to, black_to_move ? NewPiece(PAWN, WHITE) : NewPiece(PAWN, BLACK));
        }

        MovePiece(to, from);

        if (move_type == EN_PASSANT) {
            PutPiece(black_to_move ? to + SOUTH : to + NORTH, captured_piece);
        } else if (captured_piece != NO_PIECE) {
            PutPiece(to, captured_piece);
        } else if (move_type == CASTLING) {
            Square castling_rook_orig = RookFromCastling(to);
            Square castling_rook_dest = RookToCastling(to);
            MovePiece(castling_rook_dest, castling_rook_orig);
        }

        BoardData game_info = gs_history.back();
        gs_history.pop_back();
        game_state = game_info.game_state;
        checkers = game_info.checkers;
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
        ret.append("\nEP square: ");
        if (EpSquare() != SQUARE_ZERO) { ret.append(std::to_string(EpSquare())); }
        else { ret.push_back('-'); }
        return ret;
    }


}
