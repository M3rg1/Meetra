#ifndef POPPER_MISC_H
#define POPPER_MISC_H

#include "Types.h"

namespace Popper{

    inline std::string GetLogo(){
        return " ___                       \n"
               "| . \\___ ___ ___ ___ _ _  \n"
               "|  _/ . | . | . / ._| '_>  \n"
               "|_| \\___|  _|  _\\___|_|  \n"
               "        |_| |_|            \n"
               "===========================\n";
    }

    inline constexpr Piece CharToPiece(char c){
        switch(c){
            case 'P': return W_PAWN;
            case 'N': return W_KNIGHT;
            case 'B': return W_BISHOP;
            case 'R': return W_ROOK;
            case 'Q': return W_QUEEN;
            case 'K': return W_KING;
            case 'p': return B_PAWN;
            case 'n': return B_KNIGHT;
            case 'b': return B_BISHOP;
            case 'r': return B_ROOK;
            case 'q': return B_QUEEN;
            case 'k': return B_KING;
            default: return NO_PIECE;
        }
    }

    inline constexpr char PieceToChar(Piece p){
        switch(p){
            case W_PAWN: return 'P';
            case W_KNIGHT : return 'N';
            case W_BISHOP : return 'B';
            case W_ROOK : return 'R';
            case W_QUEEN : return 'Q';
            case W_KING : return 'K';
            case B_PAWN : return 'p';
            case B_KNIGHT : return 'n';
            case B_BISHOP : return 'b';
            case B_ROOK : return 'r';
            case B_QUEEN : return 'q';
            case B_KING : return 'k';
            default: return 'o';
        }
    }

}

#endif //POPPER_MISC_H
