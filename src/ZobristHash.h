#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H

#include "Types.h"

namespace Meetra {

    class Board;

    namespace Zobrist {

        void Init();

        [[nodiscard]] ZobristHash GenHash(const Board &board);
        [[nodiscard]] inline Key32 Make32Key(ZobristHash zobrist_hash) { return zobrist_hash >> 32; }
        [[nodiscard]] inline uint64_t Make44Key(ZobristHash zobrist_hash) { return zobrist_hash >> 20; }

        void PutPiece(ZobristHash &h, Piece p, Square s);
        void RemovePiece(ZobristHash &h, Piece p,Square s);
        void MovePiece(ZobristHash &h, Piece p, Square from, Square to);
        void RemoveEp(ZobristHash &h, Square s);
        void AddEp(ZobristHash &h, Square s);
        void UpdateCr(ZobristHash &h, Bitboard previous, Bitboard current);
        void UpdateColor(ZobristHash &h, Color to_move);

    }
}


#endif //MEETRA_ZOBRISTHASH_H
