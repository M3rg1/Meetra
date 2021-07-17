#include "ZobristHash.h"
#include <random>
#include "Bitboards.h"
#include "Board.h"

namespace Meetra::Zobrist {

    uint64_t piece_keys[64][12];
    uint64_t castling_keys[16];
    uint64_t ep_keys[8];
    uint64_t to_move_keys[2];


    void Init() {

        std::random_device rd;
        std::mt19937_64 mt(rd());
        std::uniform_int_distribution<uint64_t> distribution;

        for (auto &piece_types : piece_keys) {
            for (auto &key : piece_types) {
                key = distribution(mt);
            }
        }
        for (auto &key : castling_keys) {
            key = distribution(mt);
        }
        for (auto &key : ep_keys) {
            key = distribution(mt);
        }
        for (auto &key : to_move_keys) {
            key = distribution(mt);
        }
    }

    void AddPiece(ZobristHash &h, PieceType p, Color c, Square s){
        h ^= piece_keys[s][c == WHITE ? p - 1 : p + 5];
    }

    void RemovePiece(ZobristHash &h, PieceType p, Color c, Square s){
        AddPiece(h, p, c, s);
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

    void SetCr(ZobristHash &h, CastlingRights cr){
        h ^= castling_keys[cr >> 6];
    }

    void UpdateColor(ZobristHash &h, Color to_move){
        h ^= to_move_keys[OtherColor(to_move)] ^ to_move_keys[to_move];
    }

    void MovePiece(ZobristHash &h, PieceType p, Color c, Square from, Square to){
        RemovePiece(h, p, c, from);
        AddPiece(h, p, c, to);
    }

    ZobristHash GenHash(Board &board) {

        ZobristHash hash = NEW_HASH;

        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][pt - 1];
            }
            pieces = board.GetPieces(pt, BLACK);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                hash ^= piece_keys[s][pt + 5];
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