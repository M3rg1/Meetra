#include <iostream>
#include "Uci.h"
#include "Perft.h"
#include "Search.h"
#include "TestSuite.h"
#include <algorithm>
#include <unistd.h>

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
            << "option name OwnBook type check default false\n"
            << "option name Threads type spin default " << DEFAULT_SEARCH_THREADS << " min 1 max " << MAX_SEARCH_THREADS
            << "\n"
            << "option name UCI_Chess960 type check default false";
        return oss.str();
    }

    void UciCommand();
    void IsReadyCommand();
    void GoCommand(std::istringstream &iss, const Board &board);
    void UciNewGameCommand();
    void PositionCommand(std::istringstream &iss, Board &board);
    void PerftCommand(std::istringstream &iss, Board &board);
    void SetOptionCommand(std::istringstream &iss);
    void StopCommand();
    void BoardCommand(const Board &board);
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

        if (isatty(STDOUT_FILENO)) {
            Send(GetLogo() + "\n"
                 + " v. " + GetVersion() + "\n"
                 + " Made by " + GetAuthor() + "\n\n"
                 + board.PPBoard());
        }

        std::string token;
        std::string input;

        do {

            token.clear();
            std::getline(std::cin, input);
            std::istringstream iss(input);
            iss >> token;

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
            else if (token.empty()) continue;
            else UnknownCommand();

        } while (token != "quit" && !std::cin.eof());

    }

    void BoardCommand(const Board &board) {
        Uci::Send('\n' + board.PPBoard());
    }

    void TestCommand() {
        TestSuite::RunPerftTests();
    }

    void QuitCommand() {
        Search::Shutdown();
    }

    void UnknownCommand() {
        Send("Unknown command, please see the engine documentation for available commands.");
    }

    void Send(std::string_view data) {

        static std::mutex mtx;
        std::scoped_lock lock(mtx);

        std::cout << data << std::endl;
    }

    void SendInfo(const std::string &data) {
        Send("info string " + data);
    }

    void UciCommand() {
        Send(
                "id name " + GetName() + " v. " + GetVersion() + '\n'
                + "id author " + GetAuthor() + "\n"
                + GetOptions() + "\n"
                + "uciok"
        );
    }

    void GoCommand(std::istringstream &iss, const Board &board) {
        if (Search::Run()) {
            return;
        }

        Search::SearchSettings settings = ParseSearchOptions(iss);
        Search::StartSearch(settings, board);
    }

    void PerftCommand(std::istringstream &iss, Board &board) {
        Depth depth;
        iss >> depth;
        RunPerft(depth, board);
    }

    void IsReadyCommand() {
        Send("readyok");
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
            SendInfo("Invalid fen string!");
            return;
        }

        while (iss >> token) {
            if (!board.MakeUciMove(token)) {
                SendInfo("Invalid move: " + token);
                return;
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
            SendInfo("Cannot change settings while search is ongoing!");
        }

        std::string token;
        iss >> token; // name

        std::string option;
        while (iss >> token && token != "value") {
            option += token + " ";
        }

        if (option.empty()) {
            SendInfo("Invalid option name");
            return;
        }

        option.pop_back();
        std::ranges::transform(option, option.begin(), tolower);

        std::string value;
        iss >> value;

        if (!value.empty() && value != "true" && value != "false" &&
            value.find_first_not_of("0123456789") == std::string_view::npos) {
            SendInfo("Invalid option value " + value);
            return;
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
        } else if (option == "threads") {
            Search::SetNumThreads(std::stoi(value));
        } else if (option == "show current move") {
            Search::ShowCurrMoveInfo(value == "true");
        } else if (option == "ownbook") {
            Search::SetUseBook(value == "true");
        } else if (option == "uci_chess960") {
            Search::SetChess960(value == "true");
        } else {
            SendInfo("Unknown option: " + option);
        }
    }

    Search::SearchSettings ParseSearchOptions(std::istringstream &iss) {

        Search::SearchSettings settings;
        std::string option;

        while (iss >> option) {

            if (option == "wtime") iss >> settings.white_time;
            else if (option == "btime") iss >> settings.black_time;
            else if (option == "winc") iss >> settings.white_increment;
            else if (option == "binc") iss >> settings.black_increment;
            else if (option == "movetime") {
                settings.fixed_time = true;
                iss >> settings.allowed_time;
            } else if (option == "infinite") {
                settings.infinite = true;
            } else if (option == "depth") {
                iss >> settings.max_allowed_depth;
                settings.fixed_depth = true;
            } else if (option == "movestogo") {
                iss >> settings.moves_to_go;
            } else {
                SendInfo("Unknown search option: " + option);
            }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
            //else if movestogo - thats when we get time increment
        }

        return settings;
    }
}
