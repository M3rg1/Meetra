#ifndef MEETRA_UCIHANDLER_H
#define MEETRA_UCIHANDLER_H

#include "Board.h"
#include "StringTokenStream.h"
#include "Search.h"

namespace Meetra {

    class UciHandler {

    public:
        UciHandler();
        void Listen();

    private:
        void UciCommand();
        void IsReadyCommand();
        void GoCommand(StringTokenStream &sts);
        void UciNewGameCommand();
        void PositionCommand(StringTokenStream &sts);
        void PerftCommand(StringTokenStream &sts);
        void StopCommand();
        void QuitCommand();

        Board board;
        bool listen;
        ABSearch search;
        Timer info_pooling_timer;
    };

}

#endif //MEETRA_UCIHANDLER_H
