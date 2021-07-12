#include "ZobristHash.h"
#include <random>
#include "Types.h"
#include "Bitboards.h"
#include "Board.h"

namespace Meetra{

    uint64_t piece_keys[64][12];
    uint64_t castling_keys[16];
    uint64_t ep_keys[8];
    uint64_t to_move_keys[2];


    void InitZobrist(){

        std::random_device rd;
        std::mt19937_64 mt(rd());
        std::uniform_int_distribution<uint64_t> distribution;

        for(auto& piece_types : piece_keys){
            for(auto& key : piece_types){
                key = distribution(mt);
            }
        }
        for(auto& key : castling_keys){
            key = distribution(mt);
        }
        for(auto& key : ep_keys){
            key = distribution(mt);
        }
        for(auto& key : to_move_keys){
            key = distribution(mt);
        }
    }

    ZobristHash GenZobristHash(Board &board){

        ZobristHash hash = 0;

        for(PieceType pt = PAWN; pt <= KING; ++pt){
            Bitboard pieces = board.GetPieces(pt, WHITE);
            while(pieces){
                Square s = PopLsb(pieces);
                hash ^= piece_keys[s][pt - 1];
            }
            pieces = board.GetPieces(pt, BLACK);
            while(pieces){
                Square s = PopLsb(pieces);
                hash ^= piece_keys[s][pt + 5];
            }
        }

        if(board.EpSquare()){
            hash ^= ep_keys[FileFromSquare(board.EpSquare())];
        }
        hash ^= castling_keys[board.GetCR() >> 6];
        hash ^= to_move_keys[board.ColorToMove()];

        return hash;
    }



}