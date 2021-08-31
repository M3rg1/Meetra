#include <iostream>
#include "Uci.h"
#include "Perft.h"
#include "Search.h"
#include "Utils.h"
#include "TestSuite.h"

namespace Meetra::Uci {

    std::string GetLogo() {
        return " __  __         _\n"               \
               "|  \\/  |___ ___| |_ _ _ __ _\n"   \
               "| |\\/| / -_) -_)  _| '_/ _` |\n"  \
               "|_|  |_\\___\\___|\\__|_| \\__,_|";
    }
    std::string GetName() { return "Meetra"; }
    std::string GetVersion() { return "0.0.1"; }
    std::string GetAuthor() { return "M3rg1"; }
    std::string GetOptions() {
        std::ostringstream oss;
        oss << "option name Clear Hash type button\n"
            << "option name UCI_ShowCurrLine type check default false\n"
            << "option name Show current move type check default true\n"
            << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE << " max "
            << MAX_HASH_SIZE << "\n"
            << "option name MultiPV type spin default 1 min 1 max 32\n"
            << "option name Mute plies type spin default 1 min 1 max 64\n"
            << "option name Cores type spin default " << DEFAULT_SEARCH_THREADS << " min 1 max " << MAX_SEARCH_THREADS;
        return oss.str();
    }

    void UciCommand();
    void IsReadyCommand();
    void GoCommand(std::istringstream &iss, Board &board);
    void UciNewGameCommand();
    void PositionCommand(std::istringstream &iss, Board &board);
    void PerftCommand(std::istringstream &iss, Board &board);
    void SetOptionCommand(std::istringstream &iss);
    void StopCommand();
    void BoardCommand(Board &board);
    void TestCommand();
    void QuitCommand();
    void UnknownCommand();
    Search::SearchSettings ParseSearchOptions(std::istringstream &iss);

    void Init() {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }

    void Listen() {

        Board board;

        /*if (isatty(STDOUT_FILENO)) {
            SendToGui(GetLogo() + "\n"
                      + " v. " + GetVersion() + "\n"
                      + " Made by " + GetAuthor() + "\n\n"
                      + board.PPBoard());
        }*/

        std::string token;
        std::string input;

        do {

            std::getline(std::cin, input);
            std::istringstream iss(input);
            iss >> token;

            try {

                if (token == "uci") UciCommand();
                else if (token == "isready") IsReadyCommand();
                else if (token == "go") GoCommand(iss, board);
                else if (token == "position") PositionCommand(iss, board);
                else if (token == "setoption") SetOptionCommand(iss);
                else if (token == "stop") StopCommand();
                else if (token == "ucinewgame") UciNewGameCommand();
                else if (token == "perft") PerftCommand(iss, board);
                else if (token == "board") BoardCommand(board);
                else if (token == "test") TestCommand();
                else if (token == "quit") QuitCommand();
                else UnknownCommand();

            } catch (std::exception &e) {
                Uci::SendToGui(e.what());
            }

        } while (token != "quit" && !std::cin.eof());

    }

    void BoardCommand(Board &board) {
        Uci::SendToGui(board.PPBoard());
    }

    void TestCommand() {
        TestSuite::RunPerftTests();
    }

    void QuitCommand() {
        Search::Shutdown();
    }

    void UnknownCommand() {
        SendToGui("Unknown command, please see the engine documentation for available commands.");
    }

    void SendToGui(const std::string &data) {

        static std::mutex mtx;
        std::scoped_lock lock(mtx);

        std::cout << data << std::endl;
    }

    void UciCommand() {
        SendToGui(
                "id name " + GetName() + " v. " + GetVersion() + '\n'
                + "id author " + GetAuthor() + "\n"
                + GetOptions() + "\n"
                + "uciok"
        );
    }

    void GoCommand(std::istringstream &iss, Board &board) {
        if (Search::Run()) {
            return;
        }

        Search::SearchSettings settings = ParseSearchOptions(iss);
        Search::StartSearch(settings, board);
    }

    void PerftCommand(std::istringstream &iss, Board &board) {
        std::string token;
        if (iss >> token) {
            Depth depth = std::stoi(token);
            RunPerft(depth, board);
        }
    }

    void IsReadyCommand() {
        SendToGui("readyok");
    }

    void PositionCommand(std::istringstream &iss, Board &board) {

        std::string token;
        std::string fen;

        iss >> token;
        if (token == "startpos") {
            fen = STARTPOS_FEN;
            iss >> token; // moves
        } else if (token == "fen") {
            while ((iss >> token) && token != "moves") {
                fen += token + ' ';
            }
        }

        if (!board.NewPosition(fen)) {
            throw std::invalid_argument("Invalid fen string!");
        }

        if (token == "moves") {
            while (iss >> token) {
                if (!board.MakeUciMove(token)) {
                    throw std::invalid_argument("Invalid move: " + token);
                }
            }
        }
    }

    void StopCommand() {
        Search::StopSearch();
        while (!Search::Finished()); // wait until search is finished before accepting more commands
    }

    void UciNewGameCommand() {
        Search::ClearTT();
    }

    void SetOptionCommand(std::istringstream &iss) {

        if (!Search::Finished()) {
            throw std::invalid_argument("Cannot change settings while search is ongoing!");
        }

        std::string token;
        iss >> token; // name

        std::string option;
        while (iss >> token && token != "value") {
            option += token + " ";
        }

        option.pop_back();
        std::transform(option.begin(), option.end(), option.begin(), ::tolower);

        std::string value;
        iss >> value;

        try {

            if (!value.empty() && value != "true" && value != "false" && !Utils::IsPositiveNumber(value)) {
                throw std::invalid_argument("Invalid option value!");
            }

            if (option == "hash") {
                Search::SetTTSize(std::stoi(value));
            } else if (option == "clear hash") {
                Search::ClearTT();
            } else if (option == "multipv") {
                Search::SetMultiPv(std::stoi(value));
            } else if (option == "uci_showcurrline") {
                Search::ShowShowCurrLine(value == "true");
            } else if (option == "mute plies") {
                Search::SetPliesMuted(std::stoi(value));
            } else if (option == "cores") {
                Search::SetNumThreads(std::stoi(value));
            } else if (option == "show current move") {
                Search::ShowCurrMoveInfo(value == "true");
            } else {
                throw std::domain_error("Unknown option: " + option);
            }

        } catch (const std::invalid_argument &e) {
            throw std::invalid_argument("Invalid option value!");
        } catch (const std::domain_error &e) {
            throw;
        } catch (const std::out_of_range &e) {
            throw;
        }
    }

    Search::SearchSettings ParseSearchOptions(std::istringstream &iss) {

        Search::SearchSettings settings;
        std::string option;
        std::string value;

        while (iss >> option) {

            iss >> value;

            if (!Utils::IsPositiveNumber(value)) {
                throw std::invalid_argument("Invalid value: " + value + " for option: " + option);
            }

            if (option == "wtime") settings.white_time = std::stoi(value);
            else if (option == "btime") settings.black_time = std::stoi(value);
            else if (option == "winc") settings.white_increment = std::stoi(value);
            else if (option == "binc") settings.black_increment = std::stoi(value);
            else if (option == "movetime") {
                settings.fixed_time = true;
                settings.allowed_time = std::stoi(value);
            } else if (option == "infinite") {
                settings.infinite = true;
            } else if (option == "depth") {
                settings.max_allowed_depth = std::stoi(value);
                settings.fixed_depth = true;
            } else if (option == "movestogo") {

            } else {
                throw std::invalid_argument("Invalid search option: " + option);
            }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
            //else if movestogo - thats when we get time increment
        }

        return settings;
    }
}
