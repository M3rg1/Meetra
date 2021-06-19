#include "Board.h"

#include <utility>
#include "Bitboards.h"
#include "FenLoader.h"
#include "Macros.h"
#include "Misc.h"
#include <cstring>

namespace Popper {

    Board::Board(std::string fen) {
        auto li = Popper::FenLoader::ParseFen(std::move(fen));
        game_state = NEW_GAME_STATE;
        SetColorToMove(li->color_to_move, game_state);
        //SetCastlingRights();
        SetEpSquare(li->ep_square, game_state);
        SetPly(li->ply, game_state);
        SetMoveNumber(li->full_move_count, game_state);
        Piece * p;
        std::memcpy(board, li->board_occ, sizeof(Piece) * SQUARE_NR);
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
        ret.append("  | A  B  C  D  E  F  G  H");
        return ret;
    }



}
