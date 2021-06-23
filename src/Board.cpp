#include "Board.h"

#include <utility>
#include "Bitboards.h"
#include "FenLoader.h"
#include "Macros.h"
#include "Misc.h"
#include <cstring>

namespace Meetra {

    Board::Board(std::string fen) {
        gs_history = std::deque<GameState>(100);
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

        for(Square s = A1; s <= H8; ++s){
            Piece p = board[s];
            if(p != NO_PIECE){
                Bitboard pos = SquareToBB(s);
                color_bbs[ColorOfPiece(p)] |= pos;
                type_bbs[TypeOfPiece(p)] |= pos;
                type_bbs[ALL_TYPES] |= pos;
            }
        }
    }

    bool Board::MakeMove(Move m) {

        gs_history.push_back(game_state);

        bool white_moved = ColorToMove() == WHITE;
        ChangeColorToMove();
        ClearCapturedPiece();
        ClearEpSquare();

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        Square capture_square = to;
        Piece captured_piece = board[to];
        Piece moved_piece = board[from];

        if (white_moved) {
            if (moved_piece == W_ROOK) {
                if (from == A1) {
                    RemoveCastlingRights(WHITE_LONG);
                } else if (from == H1) {
                    RemoveCastlingRights(WHITE_SHORT);
                }
            } else if (moved_piece == W_KING) {
                RemoveCastlingRights(WHITE_ALL_CR);
            }
        } else {
            IncrementMoveNumber();

            if (moved_piece == B_ROOK) {
                if (from == A8) {
                    RemoveCastlingRights(BLACK_LONG);
                } else if (from == H8) {
                    RemoveCastlingRights(BLACK_SHORT);
                }
            } else if (moved_piece == B_KING) {
                RemoveCastlingRights(BLACK_ALL_CR);
            }
        }

        MoveType move_type = GetFlag(m);
        if (move_type == EN_PASSANT) {
            capture_square += white_moved ? -8 : 8;
            captured_piece = white_moved ? B_PAWN : W_PAWN;
        }

        if (captured_piece != NO_PIECE) {
            RemovePiece(capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        } else if (moved_piece != W_PAWN && moved_piece != B_PAWN) {
            ResetPly();
        } else {
            IncrementPly();
        }
        MovePiece(from, to);

        if (move_type == TWO_FORWARD) {
            SetEpSquare(white_moved ? to - 8 : to + 8);
        } else if (move_type == CASTLING) {
            if (white_moved) {
                RemoveCastlingRights(WHITE_ALL_CR);
                if (to == G1) {
                    MovePiece(H1, F1);
                } else {
                    MovePiece(A1, D1);
                }
            } else {
                RemoveCastlingRights(BLACK_ALL_CR);
                if (to == G8) {
                    MovePiece(H8, F8);
                } else {
                    MovePiece(A8, D8);
                }
            }
        } else if (IsPromotion(m)) {
            RemovePiece(to);
            switch (move_type) {
                case PROMOTE_QUEEN:
                    PutPiece(to, white_moved ? W_QUEEN : B_QUEEN);
                    break;
                case PROMOTE_ROOK:
                    PutPiece(to, white_moved ? W_ROOK : B_ROOK);
                    break;
                case PROMOTE_BISHOP:
                    PutPiece(to, white_moved ? W_BISHOP : B_BISHOP);
                    break;
                case PROMOTE_KNIGHT:
                    PutPiece(to, white_moved ? W_KNIGHT : B_KNIGHT);
                    break;
            }
        }

        // check if king in check, return false

        return true;
    }


    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        bool white_to_move = ColorToMove() == WHITE ? B_PAWN : W_PAWN;

        if (IsPromotion(m)) {
            RemovePiece(to);
            PutPiece(to, white_to_move ? B_PAWN : W_PAWN);
        }

        Piece captured_piece = CapturedPiece();

        MovePiece(to, from);
        if (captured_piece != NO_PIECE) {
            PutPiece(to, captured_piece);
        } else if (GetFlag(m) == CASTLING) {
            if (white_to_move) {
                if (to == G8) {
                    MovePiece(F8, H8);
                } else {
                    MovePiece(D8, A8);
                }
            } else {
                if (to == G1) {
                    MovePiece(F1, H1);
                } else {
                    MovePiece(D1, A1);
                }
            }
        }

        game_state = gs_history.back();
        gs_history.pop_back();
    }

    inline void Board::RemovePiece(Square s) {
        Piece p = board[s];
        board[s] = NO_PIECE;
        Bitboard pos = SquareToBB(s);
        color_bbs[ColorOfPiece(p)] ^= pos;
        type_bbs[TypeOfPiece(p)] ^= pos;
        type_bbs[ALL_TYPES] ^= pos;
    }

    inline void Board::PutPiece(Square s, Piece p) {
        board[s] = p;
        Bitboard pos = SquareToBB(s);
        color_bbs[ColorOfPiece(p)] |= pos;
        type_bbs[TypeOfPiece(p)] |= pos;
        type_bbs[ALL_TYPES] |= pos;
    }

    inline void Board::MovePiece(Square from, Square to) {
        Piece p = board[from];
        board[to] = p;
        board[from] = NO_PIECE;
        Bitboard from_to = SquareToBB(from) | SquareToBB(to);
        color_bbs[ColorOfPiece(p)] ^= from_to;
        type_bbs[TypeOfPiece(p)] ^= from_to;
        type_bbs[ALL_TYPES] ^= from_to;
    }

    std::string Board::PPBoard() const {
        std::string ret;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            ret.append(std::to_string(r + 1));
            ret.append(" |");
            for (File f = FILE_A; f <= FILE_H; ++f) {
                //DEBUG_LOG(board[SquareFromFiRa(f, r)]);
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
