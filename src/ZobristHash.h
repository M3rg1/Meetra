#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H

#include "Defs.h"

namespace Meetra {

    class Board;

    namespace Zobrist {

        void Init();

        [[nodiscard]] Hash64 GenHash64(const Board &board);
        [[nodiscard]] inline Hash32 MakeHash32(Hash64 hash64) { return hash64 >> 32; }

        void PutPiece(Hash64 &h, Piece p, Square s);
        void RemovePiece(Hash64 &h, Piece p, Square s);
        void MovePiece(Hash64 &h, Piece p, Square from, Square to);
        void RemoveEp(Hash64 &h, Square s);
        void AddEp(Hash64 &h, Square s);
        void UpdateCr(Hash64 &h, Bitboard previous, Bitboard current);
        void UpdateColor(Hash64 &h);

    }
}

#endif //MEETRA_ZOBRISTHASH_H
