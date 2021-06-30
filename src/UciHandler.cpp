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

        } while (listen && !std::cin.eof());
    }

    void UciHandler::UciCommand() {
        std::cout << "id name " << GetName() << " v. " << GetVersion() << std::endl;
        std::cout << "id author " << GetAuthor() << std::endl;
        std::cout << "options " << GetOptions() << std::endl;
    }

    void UciHandler::GoCommand(StringTokenStream &sts) {
        if(sts.HasNext()){
            std::string token = sts.NextToken();
            if(token == "perft"){
                Perft(std::stoi(sts.NextToken()), board);
            }
        }else{
            // TODO make a threadpool to send tasks to, creating a new thread every time is stewpid
            if(!IsSearching()) {
                std::jthread search_thread(StartSearch, board, 5);
            }
        }
    }

    void UciHandler::IsReadyCommand() {
        std::cout << "readyok" << std::endl;
    }

    void UciHandler::PositionCommand(StringTokenStream &sts) {
        std::string token = sts.NextToken();
        std::string fen;
        if (token == "startpos") {
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        } else {
            fen = sts.NextToken();
        }
        board = Board(fen);
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
