#include <random>
#include "ZobristHash.h"
#include "Bitboards.h"
#include "Board.h"
#include <algorithm>
#include "Uci.h"

namespace Meetra::Zobrist {

    uint64_t piece_keys[64][12];
    uint64_t castling_keys[16];
    uint64_t ep_keys[8];
    uint64_t to_move_keys[2];

    void Init() {

        auto seed = 7299078832781365792;
        std::mt19937_64 mt(seed);
        auto gen = [&](){ return mt(); };

        for (auto &piece_types : piece_keys) {
            std::ranges::generate(piece_types, gen);
        }
        std::ranges::generate(castling_keys, gen);
        std::ranges::generate(ep_keys, gen);
        std::ranges::generate(to_move_keys, gen);
    }

    void PutPiece(ZobristHash &h, Piece p, Square s){
        h ^= piece_keys[s][NumFromPiece(p)];
    }

    void RemovePiece(ZobristHash &h, Piece p, Square s){
        PutPiece(h, p, s);
    }

    void AddEp(ZobristHash &h, Square s) {
        h ^= ep_keys[FileFromSquare(s)];
    }

    void RemoveEp(ZobristHash &h, Square s){
        AddEp(h, s);
    }

    void UpdateCr(ZobristHash &h, CastlingRights previous, CastlingRights current){
        h ^= castling_keys[previous >> 6] ^ castling_keys[current >> 6];
    }

    void UpdateColor(ZobristHash &h, Color to_move){
        h ^= to_move_keys[OtherColor(to_move)] ^ to_move_keys[to_move];
    }

    void MovePiece(ZobristHash &h, Piece p, Square from, Square to){
        RemovePiece(h, p, from);
        PutPiece(h, p, to);
    }

    ZobristHash GenHash(const Board &board) {

        ZobristHash hash = NEW_HASH;

        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][NumFromPieceType<WHITE>(pt)];
            }
            pieces = board.GetPieces(pt, BLACK);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][NumFromPieceType<BLACK>(pt)];
            }
        }

        if (board.EpSquare()) {
            hash ^= ep_keys[FileFromSquare(board.EpSquare())];
        }
        hash ^= castling_keys[board.GetCR() >> 6];
        hash ^= to_move_keys[board.ColorToMove()];

        return hash;
    }


}