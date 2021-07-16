#ifndef MEETRA_FENLOADER_H
#define MEETRA_FENLOADER_H

#include "Types.h"
#include <memory>

namespace Meetra {

    class FenLoader {

    public:
        struct LoadedInfo {
            Piece board_occ[SQUARE_NR];
            Color color_to_move;
            bool w_castle_short;
            bool w_castle_long;
            bool b_castle_short;
            bool b_castle_long;
            Square ep_square;
            int ply;
            int full_move_count;
        };

        static std::unique_ptr<LoadedInfo> ParseFen(const std::string& fen);

    };


}


#endif //MEETRA_FENLOADER_H
