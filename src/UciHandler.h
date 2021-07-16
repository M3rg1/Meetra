#ifndef MEETRA_UCIHANDLER_H
#define MEETRA_UCIHANDLER_H

#include "Board.h"
#include "StringTokenStream.h"
#include "mutex"
#include "Search.h"

namespace Meetra {

    class UciHandler {

    public:
        static void Listen();
        static void SendToGui(const std::string &data);

    private:
        static void UciCommand();
        static void IsReadyCommand();
        static void GoCommand(StringTokenStream &sts, Board &board, ABSearch &search);
        static void UciNewGameCommand(ABSearch &search);
        static void PositionCommand(StringTokenStream &sts, Board &board);
        static void PerftCommand(StringTokenStream &sts, Board &board);
        static void SetOptionCommand(StringTokenStream &sts, ABSearch &search);
        static void StopCommand(ABSearch &search);
        static void QuitCommand(ABSearch& search);

        static ABSearch::SearchSettings InitSearchOptions(StringTokenStream &sts);
        static void MakeUciMove(const std::string &move_string, Board &board);
        static void ParseSearchOptions(StringTokenStream &sts, ABSearch::SearchSettings &settings);

        inline static std::mutex output_mtx;
        //inline static ABSearch search;
    };

}

#endif //MEETRA_UCIHANDLER_H
