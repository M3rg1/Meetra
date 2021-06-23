#include "Board.h"

#include <utility>
#include "Bitboards.h"
#include "FenLoader.h"
#include "Macros.h"
#include "Misc.h"
#include <cstring>

namespace Meetra {

    Board::Board(std::string fen) {
        gs_history = std::deque<GameState>(80);
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
    }

    bool Board::MakeMove(Move m) {

        gs_history.push_back(game_state);

        ClearCapturedPiece();

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        Piece captured_piece = board[to];
        board[to] = board[from];
        board[from] = NO_PIECE;

        bool white_moved = ColorToMove() == WHITE;

        if (!white_moved) IncrementMoveNumber();
        ChangeColorToMove();

        MoveType move_type = GetFlag(m);
        switch (move_type) {
            case NO_FLAG:
                break;
            case TWO_FORWARD:
                SetEpSquare(white_moved ? to - 8 : to + 8);
                break;
            case CASTLING:
                break;
            case EN_PASSANT:
                captured_piece = white_moved ? B_PAWN : W_PAWN;
                board[white_moved ? to - 8 : to + 8] = NO_PIECE;
                break;
            case PROMOTE_QUEEN:
                board[to] = white_moved ? W_QUEEN : B_QUEEN;
                break;
            case PROMOTE_ROOK:
                board[to] = white_moved ? W_ROOK : B_ROOK;
                break;
            case PROMOTE_BISHOP:
                board[to] = white_moved ? W_BISHOP : B_BISHOP;
                break;
            case PROMOTE_KNIGHT:
                board[to] = white_moved ? W_KNIGHT : B_KNIGHT;
                break;
            default:
                break;
        }

        if(captured_piece == NO_PIECE) {
            IncrementPly();
        }
        else{
            ResetPly();
            SetCapturedPiece(captured_piece);
        }

        // check if king in check, return false

        return true;
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
