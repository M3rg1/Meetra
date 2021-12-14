#include "Bitboards.h"
#include "MagicNumbers.h"
#include <sstream>
#include <algorithm>
#include <ranges>

namespace Bitboards {

    /*void SetAttacks(Magic &m, Square origin, auto &generator) {
        Bitboard occ = EMPTY_BB;
        do {
            auto idx = ((occ & m.inner_mask) * m.magic_num) >> m.shift;
            m.attacks[idx] = generator(origin, occ);
            occ = (occ - m.inner_mask) & m.inner_mask;
        } while (occ);
    }

    void InitMagic(auto &magics, auto &table, auto &magic_shift, auto &magic_num, auto &generator) {
        for (Square s: Squares) {
            Bitboard inner = ((rank_mask[RANK_1] | rank_mask[RANK_8]) & ~rank_mask[SqToRank(s)])
                             | ((file_mask[FILE_A] | file_mask[FILE_H]) & ~file_mask[SqToFile(s)]);
            magics[s].shift = 64 - magic_shift[s];
            magics[s].inner_mask = generator(s, EMPTY_BB) & ~inner;
            magics[s].magic_num = magic_num[s];
            magics[s].attacks = s == A1 ? table.data() : magics[s - 1].attacks + (1 << magic_shift[s - 1]);
            SetAttacks(magics[s], s, generator);
        }
    }

    void Init() {
        InitMagic(r_magics, r_table, r_magic_shift, r_magic_num, GenRookMoves);
        InitMagic(b_magics, b_table, b_magic_shift, b_magic_num, GenBishopMoves);
    }*/

    [[maybe_unused]] std::string PPBitboard(Bitboard b) {
        std::ostringstream oss;
        for (Rank r: Ranks | std::views::reverse) {
            oss << r + 1 << " |";
            for (File f: Files) {
                if ((b >> ((r * 8) + f)) & 1) {
                    oss << " x ";
                } else {
                    oss << " o ";
                }
            }
            oss << '\n';
        }
        oss << "    A  B  C  D  E  F  G  H";
        return oss.str();
    }

}

