#include "UciHandler.h"
#include "StringTokenStream.h"

#include <iostream>

namespace Popper {

    void UciHandler::Listen() {

        std::string token;

        do {
            std::string input;
            std::getline(std::cin, input);
            Popper::StringTokenStream sts(input, true);
            token = sts.NextToken();

            if (token == "uci") UciHandler::UciCommand();
            else if (token == "isready") UciHandler::IsReadyCommand();
            else if (token == "go") UciHandler::GoCommand();
            else if (token == "position") UciHandler::PositionCommand();
            else if (token == "quit") UciHandler::QuitCommand();
            else if (token == "stop") UciHandler::StopCommand();
            else if (token == "ucinewgame") UciHandler::UciNewGameCommand();

        } while (token != "quit" && !std::cin.eof());
    }

    void UciHandler::UciCommand() {
        std::cout << "id name Popper" << std::endl;
        std::cout << "id author M3rg1" << std::endl;
        std::cout << "options" << std::endl;
    }

    void UciHandler::GoCommand() {

    }

    void UciHandler::IsReadyCommand() {
        std::cout << "readyok" << std::endl;
    }

    void UciHandler::PositionCommand() {

    }

    void UciHandler::QuitCommand() {
        // stop search and handle exitting
    }

    void UciHandler::StopCommand() {

    }

    void UciHandler::UciNewGameCommand() {

    }
}
