#ifndef POPPER_FENLOADER_H
#define POPPER_FENLOADER_H

#include "Types.h"
#include <memory>

namespace Popper {

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

        static std::unique_ptr<LoadedInfo> ParseFen(std::string fen);

    };


}


#endif //POPPER_FENLOADER_H
