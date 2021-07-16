#ifndef MEETRA_MISC_H
#define MEETRA_MISC_H

#include "Types.h"

namespace Meetra {

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_SEARCH_DEPTH Depth(128)
#define DEFAULT_SEARCH_DEPTH Depth(32)
#define DEFAULT_SEARCH_TIME 1000
#define MAX_LEGAL_MOVES 128
#define MAX_GAME_LENGTH 512
#define DEFAULT_SEARCH_THREADS 4
#define DEFAULT_UI_SPAM 1000
#define MAX_SEARCH_THREADS 16

    // i think this should e define rather than functions
    // or mby not because of GetOptions? idk will see when i implement options

    inline std::string GetLogo() {
        return "  __  __          _            \n"
               " |  \\/  |___ ___| |_ _ _ __ _ \n"
               " | |\\/| / -_) -_)  _| '_/ _  |\n"
               " |_|  |_\\___\\___|\\__|_| \\__,_|";
    }

    inline std::string GetName() {
        return "Meetra";
    }

    inline std::string GetVersion() {
        return "0.0.1";
    }

    inline std::string GetAuthor() {
        return "M3rg1";
    }

    inline std::string GetOptions() {
        return "option name Hash type spin default 64 min 8 max 512\n"
               "option name Clear Hash type button\n"
               "option name MultiPV type spin default 1 min 1 max 32\n"
               "option name UCI_ShowCurrLine type check default false\n"
               "option name Mute plies type spin default 1 min 1 max 64\n"
               "option name Cores type spin default 1 min 1 max 16\n"
               "option name Show current move type check default true";
    }

    inline constexpr Piece CharToPiece(char c) {
        switch (c) {
            case 'P':
                return W_PAWN;
            case 'N':
                return W_KNIGHT;
            case 'B':
                return W_BISHOP;
            case 'R':
                return W_ROOK;
            case 'Q':
                return W_QUEEN;
            case 'K':
                return W_KING;
            case 'p':
                return B_PAWN;
            case 'n':
                return B_KNIGHT;
            case 'b':
                return B_BISHOP;
            case 'r':
                return B_ROOK;
            case 'q':
                return B_QUEEN;
            case 'k':
                return B_KING;
            default:
                return NO_PIECE;
        }
    }

    inline constexpr char PieceToChar(Piece p) {
        switch (p) {
            case W_PAWN:
                return 'P';
            case W_KNIGHT :
                return 'N';
            case W_BISHOP :
                return 'B';
            case W_ROOK :
                return 'R';
            case W_QUEEN :
                return 'Q';
            case W_KING :
                return 'K';
            case B_PAWN :
                return 'p';
            case B_KNIGHT :
                return 'n';
            case B_BISHOP :
                return 'b';
            case B_ROOK :
                return 'r';
            case B_QUEEN :
                return 'q';
            case B_KING :
                return 'k';
            default:
                return 'o';
        }
    }

}

#endif //MEETRA_MISC_H
