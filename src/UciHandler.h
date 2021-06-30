#ifndef MEETRA_UCIHANDLER_H
#define MEETRA_UCIHANDLER_H

#include "Board.h"
#include "StringTokenStream.h"

namespace Meetra {

    class UciHandler {

    public:
        void Listen();

    private:
        void UciCommand();
        void IsReadyCommand();
        void GoCommand(StringTokenStream &sts);
        void UciNewGameCommand();
        void PositionCommand(StringTokenStream &sts);
        void StopCommand();
        void QuitCommand();


        Board board;
        bool listen;
    };

}

#endif //MEETRA_UCIHANDLER_H
