#include "ZobristHash.h"
#include "Board.h"

#include <random>
#include <algorithm>

namespace Zobrist {

    static void GenPiecesHash(Hash64 &hash, const Board &board);

    void Init() {

        std::mt19937_64 mt(ZOBRIST_SEED); // NOLINT(cert-msc51-cpp)
        auto gen = [&] { return mt(); };

        std::ranges::for_each(piece_keys, [&](auto &pt_keys) { std::ranges::generate(pt_keys, gen); });
        std::ranges::generate(castling_keys, gen);
        std::ranges::generate(ep_keys, gen);
        ep_keys[0] = 0;
        color_key = gen();
    }

    Hash64 GenHash64(const Board &board) {

        Hash64 hash = NEW_HASH64;

        GenPiecesHash(hash, board);
        UpdateCr(hash, EMPTY_BB, board.Cr());
        AddEp(hash, board.EpSquare());
        if (board.ColorToMove() == BLACK) {
            UpdateColor(hash);
        }

        return hash;
    }

    static void GenPiecesHash(Hash64 &hash, const Board &board) {
        for (Color c: Colors) {
            for (PieceType pt: PieceTypes) {
                Bitboard pieces = board.Pieces(pt, c);
                while (pieces) {
                    Square s = Bitboards::PopLsb(pieces);
                    AddPiece(hash, NewPiece(pt, c), s);
                }
            }
        }
    }
}
