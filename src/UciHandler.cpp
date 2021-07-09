#include "UciHandler.h"
#include "Misc.h"
#include <string>
#include <iostream>
#include "MoveGen.h"
#include "Perft.h"
#include "ThreadPool.h"

namespace Meetra {

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

    void UciHandler::UciCommand() {
        std::cout << "id name " << GetName() << " v. " << GetVersion() << std::endl;
        std::cout << "id author " << GetAuthor() << std::endl;
        std::cout << GetOptions() << std::endl;
        std::cout << "uciok" << std::endl;
    }

    void UciHandler::GoCommand(StringTokenStream &sts) {
        if (search.IsSearching()) {
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
            else if (token == "movetime") {
                search_timer = std::stoi(sts.NextToken());
                fixed_timer = true;
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
        int time_left = board.ColorToMove() == WHITE ? white_time : black_time;
        if (time_left && !infinite && !fixed_depth) {
            int moves_made = std::min(board.MovesMadeCount() + 1, 10);
            double factor = 2.0 - moves_made / 10.0;
            double target = static_cast<double>(time_left) / 50.0 - moves_made;
            search_timer = static_cast<long>(factor * target);
        }

        ThreadPool::PushTask([&, depth, search_timer]() {
            search.StartSearch(board, depth, search_timer);
        });
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
        } else if (token == "fen") {
            while (sts.HasNext() && ((token = sts.NextToken()) != "moves")) {
                fen.append(token);
                fen.push_back(' ');
            }
        } else {
            return;
        }
        board = Board(fen);
        if (token == "moves" || sts.HasNext() && sts.NextToken() == "moves") {
            while (sts.HasNext()) {
                Move move_made = NewMoveFromName(sts.NextToken());
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
}
