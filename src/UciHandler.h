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
        void StopCommand();
        void QuitCommand();

        void InitSearchOptions(StringTokenStream &sts);
        void ResetSearchOptions();
        void InitSearchTimer();
        void MakeUciMove(const std::string& move_string);
        void ParseSearchOptions(StringTokenStream &sts);


        static std::mutex output_mtx;

        Board board;
        bool listen;
        ABSearch search;

        long search_timer;
        int white_time;
        int black_time;
        int white_increment;
        int black_increment;
        int depth;
        bool fixed_depth;
        bool infinite;
        bool fixed_timer;
    };

}

#endif //MEETRA_UCIHANDLER_H
