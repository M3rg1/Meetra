#include <memory>
#include <string>
#include <sstream>
#include <utility>
#include "Macros.h"
#include "FenLoader.h"
#include "StringTokenStream.h"
#include "Misc.h"

namespace Popper {

    std::unique_ptr<FenLoader::LoadedInfo> FenLoader::ParseFen(std::string fen) {

        // TODO make some basic validity checks for the fen, throw error if invalid

        auto loadedInfo = std::make_unique<LoadedInfo>();
        StringTokenStream sts(std::move(fen));

        // parse board position
        std::string board_pos_fen = sts.NextToken();
        File f = FILE_A;
        Rank r = RANK_8;
        for(char c : board_pos_fen){
            if(c == '/'){
                f = FILE_A;
                --r;
            }
            else if(std::isdigit(c)){
                std::stringstream ss;
                int empty_squares_count = c - '0';
                while(empty_squares_count > 0){
                    loadedInfo->board_occ[SquareFromFiRa(f, r)] = NO_PIECE;
                    --empty_squares_count;
                    ++f;
                }
            }
            else{
                loadedInfo->board_occ[SquareFromFiRa(f, r)] = CharToPiece(c);
                ++f;
            }
        }

        // parse color
        if (sts.HasNext()) {
            loadedInfo->color_to_move = sts.NextToken() == "w" ? WHITE : BLACK;
        }

        // parse castling
        loadedInfo->w_castle_short = false;
        loadedInfo->w_castle_long = false;
        loadedInfo->b_castle_short = false;
        loadedInfo->b_castle_long = false;
        if (sts.HasNext()) {
            std::string castling_rights = sts.NextToken();
            loadedInfo->w_castle_short = castling_rights.find('K') != std::string::npos;
            loadedInfo->w_castle_long = castling_rights.find('Q') != std::string::npos;
            loadedInfo->b_castle_short = castling_rights.find('k') != std::string::npos;
            loadedInfo->b_castle_long = castling_rights.find('q') != std::string::npos;
        }

        // parse en passant
        loadedInfo->ep_square = SQUARE_ZERO;
        if (sts.HasNext()) {
            std::string ep_info = sts.NextToken();
            if (ep_info != "-") {
                File file = File(ep_info[0] - 'a');
                Rank rank = Rank(ep_info[1] - '1');
                loadedInfo->ep_square = SquareFromFiRa(file, rank);
            }
        }

        // parse ply
        loadedInfo->ply = 0;
        if (sts.HasNext()) {
            std::string ply = sts.NextToken();
            if (ply != "-") {
                std::stringstream ss;
                ss << ply;
                ss >> loadedInfo->ply;
            }
        }

        // parse full move count
        loadedInfo->full_move_count = 0;
        if (sts.HasNext()) {
            std::string move_count = sts.NextToken();
            if (move_count != "-") {
                std::stringstream ss;
                ss << move_count;
                ss >> loadedInfo->full_move_count;
            }
        }

        return loadedInfo;
    }

}
