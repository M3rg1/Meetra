#include <random>
#include "ZobristHash.h"
#include "Bitboards.h"
#include "Board.h"
#include <algorithm>
#include <array>

namespace Zobrist {

    std::array<std::array<uint64_t, B_KING + 1>, SQUARE_NR> piece_keys;
    std::array<uint64_t, FILE_NR> castling_keys;
    std::array<uint64_t, FILE_NR>  ep_keys;
    uint64_t color_key;

    void Init() {

        std::mt19937_64 mt(ZOBRIST_SEED);
        auto gen = [&] { return mt(); };

        std::ranges::for_each(piece_keys, [&](auto &pt_keys) { std::ranges::generate(pt_keys, gen); });
        std::ranges::generate(castling_keys, gen);
        std::ranges::generate(ep_keys, gen);
        color_key = gen();
    }

    void AddPiece(Hash64 &h, Piece p, Square s) {
        h ^= piece_keys[s][p];
    }

    void RemovePiece(Hash64 &h, Piece p, Square s) {
        AddPiece(h, p, s);
    }

    void AddEp(Hash64 &h, Square s) {
        h ^= ep_keys[SqToFile(s)];
    }

    void RemoveEp(Hash64 &h, Square s) {
        AddEp(h, s);
    }

    void UpdateCr(Hash64 &h, Bitboard previous, Bitboard current) {
        Bitboard cr_change = previous ^ current;
        while (cr_change) {
            h ^= castling_keys[SqToFile(Bitboards::PopLsb(cr_change))];
        }
    }

    void UpdateColor(Hash64 &h) {
        h ^= color_key;
    }

    void MovePiece(Hash64 &h, Piece p, Square from, Square to) {
        h ^= piece_keys[from][p] ^ piece_keys[to][p];
    }

    Hash64 GenHash64(const Board &board) {

        Hash64 hash = NEW_HASH64;

        for (Color c: Colors) {
            for (PieceType pt: PieceTypes) {
                Bitboard pieces = board.Pieces(pt, c);
                while (pieces) {
                    Square s = Bitboards::PopLsb(pieces);
                    hash ^= piece_keys[s][NewPiece(pt, c)];
                }
            }
        }

        if (board.EpSquare()) {
            hash ^= ep_keys[SqToFile(board.EpSquare())];
        }

        Bitboard cr = board.Cr();
        while (cr) {
            hash ^= castling_keys[Bitboards::PopLsb(cr)];
        }

        if (board.ColorToMove() == BLACK) {
            hash ^= color_key;
        }

        return hash;
    }
}