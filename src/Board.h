#ifndef POPPER_BOARD_H
#define POPPER_BOARD_H


#include <string>
#include "Types.h"

namespace Popper {

    class Board {

    public:
        // functions
        explicit Board(std::string Fen);
        inline constexpr Bitboard GetPieces(Piece p);
        inline constexpr Bitboard GetPieces(PieceType pt);
        inline constexpr Bitboard GetPieces(PieceType pt, Color c);
        inline constexpr Bitboard GetPieces(Color c);
        [[nodiscard]] std::string PPBoard() const;

        // data
        // empty

    //private:
        GameState game_state { 0 };
        Piece board[SQUARE_NR] {};
        Bitboard piece_bbs[PIECE_NR];
        Bitboard color_bbs[COLOR_NR];
        Bitboard type_bbs[PIECE_TYPE_NR];
    };

    inline constexpr Bitboard Board::GetPieces(Piece p) {
        return piece_bbs[p];
    }
    inline constexpr Bitboard Board::GetPieces(PieceType pt, Color c){
        return type_bbs[pt] & color_bbs[c];
    }
    inline constexpr Bitboard Board::GetPieces(PieceType pt){
        return type_bbs[pt];
    }
    inline constexpr Bitboard Board::GetPieces(Color c){
        return color_bbs[c];
    }
}


#endif //POPPER_BOARD_H
