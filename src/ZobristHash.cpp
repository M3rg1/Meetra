#include <random>
#include "ZobristHash.h"
#include "Bitboards.h"
#include "Board.h"
#include <algorithm>

namespace Meetra::Zobrist {

    uint64_t piece_keys[SQUARE_NR][B_KING + 1];
    uint64_t castling_keys[SQUARE_NR];
    uint64_t ep_keys[RANK_NR];
    uint64_t color_key;

    void Init() {

        auto seed = 7299078832781365792;
        std::mt19937_64 mt(seed);
        auto gen = [&] { return mt(); };

        std::ranges::for_each(piece_keys, [&] (auto &pt_keys) { std::ranges::generate(pt_keys, gen); });
        std::ranges::generate(castling_keys, gen);
        std::ranges::generate(ep_keys, gen);
        color_key = gen();
    }

    void PutPiece(Hash64 &h, Piece p, Square s) {
        h ^= piece_keys[s][p];
    }

    void RemovePiece(Hash64 &h, Piece p, Square s) {
        PutPiece(h, p, s);
    }

    void AddEp(Hash64 &h, Square s) {
        h ^= ep_keys[FileFromSquare(s)];
    }

    void RemoveEp(Hash64 &h, Square s) {
        AddEp(h, s);
    }

    void UpdateCr(Hash64 &h, Bitboard previous, Bitboard current) {
        Bitboard cr_change = previous ^ current;
        while (cr_change) {
            h ^= castling_keys[Bitboards::PopLsb(cr_change)];
        }
    }

    void UpdateColor(Hash64 &h) {
        h ^= color_key;
    }

    void MovePiece(Hash64 &h, Piece p, Square from, Square to) {
        h ^= piece_keys[from][p] ^ piece_keys[to][p];;
    }

    Hash64 GenHash64(const Board &board) {

        Hash64 hash = NEW_HASH;

        for (Color c = WHITE; c < COLOR_NR; ++c) {
            for (PieceType pt = PAWN; pt < PIECE_TYPE_NR; ++pt) {
                Bitboard pieces = board.GetPieces(pt, c);
                while (pieces) {
                    Square s = Bitboards::PopLsb(pieces);
                    hash ^= piece_keys[s][NewPiece(pt, c)];
                }
            }
        }

        if (board.EpSquare()) {
            hash ^= ep_keys[FileFromSquare(board.EpSquare())];
        }

        Bitboard cr = board.GetCr();
        while(cr) {
            hash ^= castling_keys[Bitboards::PopLsb(cr)];
        }

        if (board.ColorToMove() == BLACK) {
            hash ^= color_key;
        }

        return hash;
    }


}