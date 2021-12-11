#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H

#include "Defs.h"

class Board;

namespace Zobrist {

    void Init();

    [[nodiscard]] Hash64 GenHash64(const Board &board);
    [[nodiscard]] inline Hash16 MakeHash16(Hash64 hash64) { return static_cast<Hash16>(hash64 >> 48); }

    void AddPiece(Hash64 &h, Piece p, Square s);
    void RemovePiece(Hash64 &h, Piece p, Square s);
    void MovePiece(Hash64 &h, Piece p, Square from, Square to);
    void RemoveEp(Hash64 &h, Square s);
    void AddEp(Hash64 &h, Square s);
    void UpdateCr(Hash64 &h, Bitboard previous, Bitboard current);
    void UpdateColor(Hash64 &h);

}

#endif //MEETRA_ZOBRISTHASH_H
