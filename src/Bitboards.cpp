#include "Bitboards.h"

using namespace Popper;


namespace Popper{
// https://www.chessprogramming.org/Little-endian board printing
    std::string PPStringBitboard(Bitboard b) {
        std::string ret;
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            for (File f = Popper::FILE_A; f < FILE_H; ++f) {
                ret.append(" o ");
            }
            ret.append("\n");
        }
        return ret;
    }
}