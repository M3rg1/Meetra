#ifndef MEETRA_ZOBRISTHASH_H
#define MEETRA_ZOBRISTHASH_H

#include "Defs.h"
#include <random>
#include <algorithm>
#include <array>
#include "Bitboards.h"
#include "Config.h"

class Board;


    class Zobrist {

        //inline static auto generator = [] { static std::mt19937_64 mt(ZOBRIST_SEED); return mt(); };
        inline static std::array<std::array<uint64_t, B_KING + 1>, SQUARE_NR> piece_keys = {};
        inline static std::array<uint64_t, SQUARE_NR> castling_keys = {};
        inline static std::array<uint64_t, SQUARE_NR> ep_keys = {};
        inline static uint64_t color_key = {};

    public:


        Hash64 hash_64;
        Zobrist() = default;
        void GenPiecesHash(const Board &board);

        Hash64 GenHash64(const Board &board);
        Zobrist(const Board &board) {
            GenHash64(board);
        }

        [[nodiscard]] Hash64 GetHash64() const { return hash_64; }
        [[nodiscard]] Hash16 GetHash16() const { return static_cast<Hash16>(hash_64 >> 48); }


        static void Init() {

            std::mt19937_64 mt(ZOBRIST_SEED); // NOLINT(cert-msc51-cpp)
            auto generator = [&] { return mt(); };

            std::ranges::for_each(piece_keys, [&](auto &pt_keys) { std::ranges::generate(pt_keys, generator); });
            std::ranges::generate(castling_keys, generator);
            std::ranges::generate(ep_keys, generator);
            ep_keys[0] = 0;
            color_key = generator();
        }

        void AddPiece(Piece p, Square s) {
            hash_64 ^= piece_keys[s][p];
        }

        void RemovePiece(Piece p, Square s) {
            AddPiece(p, s);
        }

        void AddEp(Square s) {
            hash_64 ^= ep_keys[s];
        }

        void RemoveEp( Square s) {
            AddEp(s);
        }

        void UpdateCr(Bitboard previous, Bitboard current) {
            Bitboard cr_change = previous ^ current;
            while (cr_change) {
                hash_64 ^= castling_keys[Bitboards::PopLsb(cr_change)];
            }
        }

        void UpdateColor() {
            hash_64 ^= color_key;
        }

        void MovePiece(Piece p, Square from, Square to) {
            hash_64 ^= piece_keys[from][p] ^ piece_keys[to][p];
        }
    };


#endif //MEETRA_ZOBRISTHASH_H
