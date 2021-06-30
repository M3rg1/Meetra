#include "UciHandler.h"
#include "Misc.h"
#include <string>
#include <iostream>
#include "Search.h"
#include "MoveGen.h"
#include "Search.h"
#include <thread>
#include "Perft.h"

namespace Meetra {

    UciHandler::UciHandler() {
        board = Board(STARTPOS_FEN);
        listen = false;
    }

    void UciHandler::Listen() {

        listen = true;
        std::string token;

        do {
            std::string input;
            std::getline(std::cin, input);
            StringTokenStream sts(input, true);
            token = sts.NextToken();

            if (token == "uci") UciHandler::UciCommand();
            else if (token == "isready") UciHandler::IsReadyCommand();
            else if (token == "go") UciHandler::GoCommand(sts);
            else if (token == "position") UciHandler::PositionCommand(sts);
            else if (token == "quit") UciHandler::QuitCommand();
            else if (token == "stop") UciHandler::StopCommand();
            else if (token == "ucinewgame") UciHandler::UciNewGameCommand();
            else if (token == "perft") UciHandler::PerftCommand(sts);

        } while (listen && !std::cin.eof());
    }

    void UciHandler::UciCommand() {
        std::cout << "id name " << GetName() << " v. " << GetVersion() << std::endl;
        std::cout << "id author " << GetAuthor() << std::endl;
        std::cout << GetOptions() << std::endl;
        std::cout << "uciok" << std::endl;
    }

    void UciHandler::GoCommand(StringTokenStream &sts) {
        // go wtime 241594 btime 252000 winc 0 binc 0
        if (IsSearching()) {
            return;
        }
        long search_timer = DEFAULT_SEARCH_TIME;
        int white_time = 0;
        int black_time = 0;
        int white_increment = 0;
        int black_increment = 0;
        int depth = DEFAULT_SEARCH_DEPTH;
        bool fixed_depth = false;
        bool infinite = false;
        bool fixed_timer = false;
        while (sts.HasNext()) {
            std::string token = sts.NextToken();
            if (token == "wtime") white_time = std::stoi(sts.NextToken());
            else if (token == "btime") black_time = std::stoi(sts.NextToken());
            else if (token == "winc") white_increment = std::stoi(sts.NextToken());
            else if (token == "binc") black_increment = std::stoi(sts.NextToken());
            else if (token == "movetime") { search_timer = std::stoi(sts.NextToken()); fixed_timer = true; }
            else if (token == "infinite") infinite = true;
            else if (token == "depth") { depth = std::stoi(sts.NextToken()); fixed_depth = true; }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
        }
        std::cout << "Wtime: " << white_time << " BTime: " << black_time << std::endl;
        int time_left = board.ColorToMove() == WHITE ? white_time : black_time;
        std::cout << "Timer: " << time_left <<  std::endl;
        if(!infinite && !fixed_depth) {
            if(!fixed_timer && time_left) {
                int moves_made = std::min(board.MovesMadeCount() + 1, 10);
                double factor = 2.0 - moves_made / 10.0;
                double target = static_cast<double>(time_left) / 50.0 - moves_made;
                search_timer = static_cast<long>(factor * target);
            }
            std::cout << "Allocated time: " << search_timer << std::endl;
            timer.SetTimeout(StopSearch, search_timer);
        }
        InitSearch();
        std::jthread search_thread(StartSearch, board, depth, search_timer);
    }

    void UciHandler::PerftCommand(StringTokenStream &sts) {
        if (sts.HasNext()) {
            int depth = std::stoi(sts.NextToken());
            RunPerft(depth, board);
        }
    }

    void UciHandler::IsReadyCommand() {
        std::cout << "readyok" << std::endl;
    }

    void UciHandler::PositionCommand(StringTokenStream &sts) {
        std::string token = sts.NextToken();
        std::string fen;
        if (token == "startpos") {
            fen = STARTPOS_FEN;
        } else {
            fen = sts.NextToken();
        }
        board = Board(fen);
        if (sts.HasNext() && sts.NextToken() == "moves") {
            while (sts.HasNext()) {
                Move move_made = NewMoveFromName(sts.NextToken());
                MoveGen move_gen(board);
                Move move;
                while ((move = move_gen.GetNextMove<false>())) {
                    if (FromSquare(move) == FromSquare(move_made) && ToSquare(move) == ToSquare(move_made)) {
                        if (IsPromotion(move) && GetFlag(move) != GetFlag(move_made)) {
                            continue;
                        }
                        board.MakeMove(move);
                        break;
                    }
                }
            }
        }
    }

    void UciHandler::QuitCommand() {
        StopSearch();
        listen = false;
    }

    void UciHandler::StopCommand() {
        StopSearch();
    }

    void UciHandler::UciNewGameCommand() {
        // reset TT
    }
}
