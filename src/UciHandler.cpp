#include "UciHandler.h"
#include "Misc.h"
#include <string>
#include <iostream>
#include <sstream>
#include "MoveGen.h"
#include "Perft.h"
#include "ThreadPool.h"
#include "Search.h"

namespace Meetra {

    std::mutex UciHandler::output_mtx;

    UciHandler::UciHandler() {
        board = Board(STARTPOS_FEN);
        listen = false;
    }

    void UciHandler::Listen() {
        listen = true;
        std::string token;
        std::string input;

        do {
            std::getline(std::cin, input);
            // TODO make lower for sts.GetNextToken - not for the entire string - so we can get the FEN properly capitalized
            // but other shit lowered
            StringTokenStream sts(input, false);
            token = sts.NextToken();

            if (token == "uci") UciCommand();
            else if (token == "isready") IsReadyCommand();
            else if (token == "go") GoCommand(sts);
            else if (token == "position") PositionCommand(sts);
            else if (token == "quit") QuitCommand();
            else if (token == "stop") StopCommand();
            else if (token == "ucinewgame") UciNewGameCommand();
            else if (token == "perft") PerftCommand(sts);

        } while (listen && !std::cin.eof());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void UciHandler::SendToGui(const std::string& data){
        std::scoped_lock<std::mutex> lock{output_mtx};
        std::cout << data << std::endl;
    }

    void UciHandler::UciCommand() {
        std::stringstream ss;
        ss << "id name " << GetName() << " v. " << GetVersion() << '\n'
           << "id author " << GetAuthor() << '\n'
           << GetOptions() << '\n'
           << "uciok";
        SendToGui(ss.str());
    }

    void UciHandler::GoCommand(StringTokenStream &sts) {
        if (search.IsSearching()) {
            return;
        }

        InitSearchOptions(sts);

        ThreadPool::PushTask([&]() {
            search.StartSearch(board, depth, search_timer, DEFAULT_SEARCH_THREADS, fixed_timer);
        });
    }

    void UciHandler::PerftCommand(StringTokenStream &sts) {
        if (sts.HasNext()) {
            depth = std::stoi(sts.NextToken());
            RunPerft(depth, board);
        }
    }

    void UciHandler::IsReadyCommand() {
        SendToGui("readyok");
    }

    void UciHandler::PositionCommand(StringTokenStream &sts) {

        std::string token = sts.NextToken();
        std::string fen;
        if (token == "startpos") {
            fen = STARTPOS_FEN;
            if (sts.HasNext()) token = sts.NextToken();
        } else if (token == "fen") {
            while (sts.HasNext() && ((token = sts.NextToken()) != "moves")) {
                fen.append(token);
                fen.push_back(' ');
            }
        }

        board = Board(fen);

        if (token == "moves") {
            while (sts.HasNext()) {
                MakeUciMove(sts.NextToken());
            }
        }
    }

    void UciHandler::QuitCommand() {
        StopCommand();
        listen = false;
        // await search shutdown
        // TODO will shutdown threadpool as well, and await here in while loop until thread pool running = false
        //  (shutdown = true - static var in destructor set to true)
    }

    void UciHandler::StopCommand() {
        search.StopSearch();
        // should await search completion here maybe?
    }

    void UciHandler::UciNewGameCommand() {
        search.ClearTT();
    }

    void UciHandler::MakeUciMove(const std::string &move_string) {
        Move move_made = NewMoveFromName(move_string);
        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetNextMove<false>())) {
            if (FromSquare(move) == FromSquare(move_made) && ToSquare(move) == ToSquare(move_made)) {
                if (IsPromotion(move) && GetMoveType(move) != GetMoveType(move_made)) {
                    continue;
                }
                board.MakeMove(move);
                break;
            }
        }
    }

    void UciHandler::InitSearchOptions(StringTokenStream &sts) {
        ResetSearchOptions();
        ParseSearchOptions(sts);
        InitSearchTimer();
    }

    void UciHandler::ParseSearchOptions(StringTokenStream &sts){
        while (sts.HasNext()) {
            std::string token = sts.NextToken();
            if (token == "wtime") white_time = std::stoi(sts.NextToken());
            else if (token == "btime") black_time = std::stoi(sts.NextToken());
            else if (token == "winc") white_increment = std::stoi(sts.NextToken());
            else if (token == "binc") black_increment = std::stoi(sts.NextToken());
            else if (token == "movetime") {
                depth = MAX_SEARCH_DEPTH;
                fixed_timer = true;
                search_timer = std::stoi(sts.NextToken());
            } else if (token == "infinite") {
                depth = MAX_SEARCH_DEPTH;
                infinite = true;
                search_timer = INFINITE_TIMER;
            } else if (token == "depth") {
                depth = std::stoi(sts.NextToken());
                fixed_depth = true;
                search_timer = INFINITE_TIMER;
            }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
        }
    }

    void UciHandler::InitSearchTimer() {

        if(infinite || fixed_depth || fixed_timer){
            return;
        }

        int time_left = board.ColorToMove() == WHITE ? white_time : black_time;
        if (time_left) {
            int moves_made = std::min(board.MovesMadeCount() + 1, 10);
            double factor = 2.0 - moves_made / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - moves_made;
            search_timer = static_cast<long>(factor * target);
        }
    }

    void UciHandler::ResetSearchOptions() {
        search_timer = DEFAULT_SEARCH_TIME;
        white_time = 0;
        black_time = 0;
        white_increment = 0;
        black_increment = 0;
        depth = DEFAULT_SEARCH_DEPTH;
        fixed_depth = false;
        infinite = false;
        fixed_timer = false;
    }

}
