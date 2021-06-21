#include "Board.h"

#include <utility>
#include "Bitboards.h"
#include "FenLoader.h"
#include "Macros.h"
#include "Misc.h"
#include <cstring>

namespace Meetra {

    Board::Board(std::string fen) {
        auto loadedInfo = Meetra::FenLoader::ParseFen(std::move(fen));
        SetColorToMove(loadedInfo->color_to_move, game_state);
        if(loadedInfo->w_castle_short) { SetCastlingRights(WHITE_SHORT, game_state); }
        if(loadedInfo->w_castle_long) { SetCastlingRights(WHITE_LONG, game_state); }
        if(loadedInfo->b_castle_short) { SetCastlingRights(BLACK_SHORT, game_state); }
        if(loadedInfo->b_castle_long) { SetCastlingRights(BLACK_LONG, game_state); }
        SetEpSquare(loadedInfo->ep_square, game_state);
        SetPly(loadedInfo->ply, game_state);
        SetMoveNumber(loadedInfo->full_move_count, game_state);
        std::memcpy(board, loadedInfo->board_occ, sizeof(Piece) * SQUARE_NR);
    }

    std::string Board::PPBoard() const {
        std::string ret;
        for(Rank r = RANK_8; r >= RANK_1; --r) {
            ret.append(std::to_string(r + 1));
            ret.append(" |");
            for(File f = FILE_A; f <= FILE_H; ++f) {
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
        ColorToMove(game_state) == WHITE ? ret.append("white\n") : ret.append("black\n");
        ret.append("Move count: ");
        ret.append(std::to_string(TotalMoves(game_state)));
        ret.append(" | Ply since last capture: ");
        ret.append(std::to_string(Ply(game_state)));
        ret.append("\nCastling rights: ");
        bool castling_available = false;
        if(CanWhiteShortCR(game_state)){ ret.push_back('K'); castling_available = true; }
        if(CanWhiteLongCR(game_state)) { ret.push_back('Q'); castling_available = true; }
        if(CanBlackShortCR(game_state)){ ret.push_back('k'); castling_available = true; }
        if(CanBlackLongCR(game_state)) { ret.push_back('q'); castling_available = true; }
        if(!castling_available){ ret.push_back('-'); }
        ret.append("\nEP square: ");
        if(EpSquare(game_state) != SQUARE_ZERO) { ret.append(std::to_string(EpSquare(game_state))); }
        else{ ret.push_back('-'); }
        return ret;
    }



}
