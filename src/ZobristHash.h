#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H


#include "Types.h"


namespace Meetra {

    class Board;

    namespace Zobrist {

        void Init();
        ZobristHash GenHash(Board &board);
        inline Key32 Make32Key(ZobristHash zobrist_hash) { return zobrist_hash >> 32; }

        void AddPiece(ZobristHash &h, PieceType p, Color c, Square s);
        void RemovePiece(ZobristHash &h, PieceType p, Color c, Square s);
        void MovePiece(ZobristHash &h, PieceType p, Color c, Square from, Square to);
        void RemoveEp(ZobristHash &h, Square s);
        void AddEp(ZobristHash &h, Square s);
        void UpdateCr(ZobristHash &h, CastlingRights previous, CastlingRights current);
        void SetCr(ZobristHash &h, CastlingRights cr);
        void UpdateColor(ZobristHash &h, Color to_move);

    }
}


#endif //MEETRA_ZOBRISTHASH_H
