#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H

#include "Defs.h"
#include "Bitboards.h"

#include <array>

class Board;

namespace Zobrist {

    inline std::array<std::array<uint64_t, B_KING + 1>, SQUARE_NR> piece_keys;
    inline std::array<uint64_t, SQUARE_NR> castling_keys;
    inline std::array<uint64_t, SQUARE_NR> ep_keys;
    inline uint64_t color_key;

    void Init();

    [[nodiscard]] Hash64 GenHash64(const Board &board);
    [[nodiscard]] constexpr Hash16 MakeHash16(Hash64 hash64) { return static_cast<Hash16>(hash64 >> 48); }

    inline void AddPiece(Hash64 &h, Piece p, Square s) { h ^= piece_keys[s][p]; }
    inline void RemovePiece(Hash64 &h, Piece p, Square s) { AddPiece(h, p, s); }
    inline void AddEp(Hash64 &h, Square s) { h ^= ep_keys[s]; }
    inline void RemoveEp(Hash64 &h, Square s) { AddEp(h, s); }
    inline void MovePiece(Hash64 &h, Piece p, Square from, Square to) { h ^= piece_keys[from][p] ^ piece_keys[to][p]; }
    inline void UpdateColor(Hash64 &h) { h ^= color_key; }
    inline void UpdateCr(Hash64 &h, Bitboard previous, Bitboard current) {
        Bitboard cr_change = previous ^ current;
        while (cr_change) {
            h ^= castling_keys[Bitboards::PopLsb(cr_change)];
        }
    }

}

#endif //MEETRA_ZOBRISTHASH_H
