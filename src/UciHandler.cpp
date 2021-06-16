#include "UciHandler.h"
#include "utils/StringTokenStream.h"

#include <iostream>

static std::string Name = "Popper";
static std::string Version = "0.0";
static std::string Author = "M3rg1";

namespace Popper {

    void UciHandler::listen(){

        std::string token;

        do{
            std::string input;
            std::getline(std::cin, input);
            Popper::StringTokenStream sts(input);
            token = sts.nextToken();

            if(token == "uci") UciHandler::uciCommand();
            else if(token == "isready") UciHandler::isReadyCommand();
            else if(token == "go") UciHandler::goCommand();
            else if(token == "position") UciHandler::positionCommand();
            else if(token == "quit") UciHandler::quitCommand();
            else if(token == "stop") UciHandler::stopCommand();
            else if(token == "ucinewgame") UciHandler::uciNewGameCommand();

        }while(token != "quit" && !std::cin.eof());
    }

    void UciHandler::uciCommand(){
        std::cout << "id name Popper" << std::endl;
        std::cout << "id author M3rg1" << std::endl;
        std::cout << "options" << std::endl;
    }

    void UciHandler::goCommand() {

    }

    void UciHandler::isReadyCommand() {
        std::cout << "readyok" << std::endl;
    }

    void UciHandler::positionCommand() {

    }

    void UciHandler::quitCommand() {
        // stop search and handle exitting
    }

    void UciHandler::stopCommand() {

    }

    void UciHandler::uciNewGameCommand() {

    }
}
