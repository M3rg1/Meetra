#include <random>
#include <algorithm>
#include <array>
#include "ZobristHash.h"
#include "Bitboards.h"
#include "Board.h"


//    std::array<std::array<uint64_t, B_KING + 1>, SQUARE_NR> piece_keys;
//    std::array<uint64_t, SQUARE_NR> castling_keys;
//    std::array<uint64_t, SQUARE_NR> ep_keys;
//    uint64_t color_key;



    void Zobrist::GenPiecesHash(const Board &board) {
        for (Color c: Colors) {
            for (PieceType pt: PieceTypes) {
                Bitboard pieces = board.Pieces(pt, c);
                while (pieces) {
                    Square s = Bitboards::PopLsb(pieces);
                    AddPiece(NewPiece(pt, c), s);
                }
            }
        }
    }

    Hash64 Zobrist::GenHash64(const Board &board) {

        Hash64 hash = NEW_HASH64;

        GenPiecesHash(board);
        UpdateCr(EMPTY_BB, board.Cr());
        AddEp(board.EpSquare());
        if (board.ColorToMove() == BLACK) {
            UpdateColor();
        }

        return hash;
    }