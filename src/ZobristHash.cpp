#include <random>
#include "ZobristHash.h"
#include "Bitboards.h"
#include "Board.h"
#include <algorithm>

namespace Meetra::Zobrist {

    uint64_t piece_keys[64][12];
    uint64_t castling_keys[64];
    uint64_t ep_keys[8];
    uint64_t to_move_keys[2];

    void Init() {

        auto seed = 7299078832781365792;
        std::mt19937_64 mt(seed);
        auto gen = [&] { return mt(); };

        std::ranges::for_each(piece_keys, [&] (auto &pt_keys) { std::ranges::generate(pt_keys, gen); });
        std::ranges::generate(castling_keys, gen);
        std::ranges::generate(ep_keys, gen);
        std::ranges::generate(to_move_keys, gen);
    }

    void PutPiece(Hash64 &h, Piece p, Square s) {
        h ^= piece_keys[s][IdxFromPiece(p)];
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

    void UpdateColor(Hash64 &h, Color to_move) {
        h ^= to_move_keys[OtherColor(to_move)] ^ to_move_keys[to_move];
    }

    void MovePiece(Hash64 &h, Piece p, Square from, Square to) {
        RemovePiece(h, p, from);
        PutPiece(h, p, to);
    }

    Hash64 GenHash64(const Board &board) {

        Hash64 hash = NEW_HASH;

        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][IdxFromPieceType<WHITE>(pt)];
            }
            pieces = board.GetPieces(pt, BLACK);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][IdxFromPieceType<BLACK>(pt)];
            }
        }

        if (board.EpSquare()) {
            hash ^= ep_keys[FileFromSquare(board.EpSquare())];
        }

        Bitboard cr = board.GetCr();
        while(cr) {
            hash ^= castling_keys[Bitboards::PopLsb(cr)];
        }

        hash ^= to_move_keys[board.ColorToMove()];

        return hash;
    }


}