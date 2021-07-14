#ifndef MEETRA_UCIHANDLER_H
#define MEETRA_UCIHANDLER_H

#include "Board.h"
#include "StringTokenStream.h"
#include "mutex"
#include "Search.h"

namespace Meetra {

    class UciHandler {

    public:
        UciHandler();
        void Listen();
        static void SendToGui(const std::string& data);

    private:
        void UciCommand();
        void IsReadyCommand();
        void GoCommand(StringTokenStream &sts);
        void UciNewGameCommand();
        void PositionCommand(StringTokenStream &sts);
        void PerftCommand(StringTokenStream &sts);
        void SetOptionCommand(StringTokenStream &sts);
        void StopCommand();
        void QuitCommand();

        ABSearch::SearchSettings InitSearchOptions(StringTokenStream &sts);
        void MakeUciMove(const std::string& move_string);
        void ParseSearchOptions(StringTokenStream &sts, ABSearch::SearchSettings &settings);


        static std::mutex output_mtx;

        Board board;
        bool listen;
        ABSearch search;
    };

}

#endif //MEETRA_UCIHANDLER_H
