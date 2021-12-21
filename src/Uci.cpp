#include <iostream>
#include "Uci.h"
#include "Search.h"
#include "TestSuite.h"
#include <Env.h>
#include <unistd.h>
#include <syncstream>

namespace Uci {

    const std::string LOGO = " __  __         _\n"               \
                             "|  \\/  |___ ___| |_ _ _ __ _\n"   \
                             "| |\\/| / -_) -_)  _| '_/ _` |\n"  \
                             "|_|  |_\\___\\___|\\__|_| \\__,_|";

    std::string Options() {
        std::ostringstream oss;
        oss << std::boolalpha
            << "option name Clear Hash type button\n"
            << "option name UCI_ShowCurrLine type check default " << DEFAULT_SHOW_CURRLINE << '\n'
            << "option name Show current move type check default " << DEFAULT_SHOW_CURRMOVE << '\n'
            << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE
            << " max " << MAX_HASH_SIZE << '\n'
            << "option name MultiPV type spin default " << DEFAULT_MULTI_PV << " min " << MIN_MULTI_PV << " max "
            << MAX_MULTI_PV << '\n'
            << "option name Mute plies type spin default " << DEFAULT_MUTE_PLIES << " min " << MIN_MUTE_PLIES << " max "
            << MAX_MUTE_PLIES << '\n'
            << "option name OwnBook type check default " << DEFAULT_USE_BOOK << '\n'
            << "option name Threads type spin default " << DEFAULT_SEARCH_THREADS << " min " << MIN_SEARCH_THREADS
            << " max " << MAX_SEARCH_THREADS << '\n'
            << "option name Move overhead type spin default " << DEFAULT_OVERHEAD << " min "
            << MIN_OVERHEAD << " max " << MAX_OVERHEAD << '\n'
            << "option name UCI_Chess960 type check default " << DEFAULT_CHESS960 << '\n'
            << "option name Send updates frequency type spin default " << DEFAULT_UPDATE_INTERVAL << " min "
            << MIN_UPDATE_INTERVAL << " max " << MAX_UPDATE_INTERVAL;
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
    void UnknownCommand();
    Search::Settings ParseSearchOptions(std::istringstream &iss);

    void Init() {
        std::ios_base::sync_with_stdio(false); // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=27931
        std::cin.tie(nullptr);
    }

    void Listen() {

        Board board;

        if (isatty(STDOUT_FILENO)) {
            std::osyncstream(std::cout)
                    << LOGO << '\n'
                    << " v. " << PROJECT_VER << '\n'
                    << " Made by " << PROJECT_AUTHOR << "\n\n"
                    << board.PrettyPrint() << std::endl;
        }

        std::string input;
        while (std::getline(std::cin, input)) {

            std::istringstream iss(std::move(input));
            std::string command;
            iss >> command;

            if (command == "uci") UciCommand();
            else if (command == "isready") IsReadyCommand();
            else if (command == "go") GoCommand(iss, board);
            else if (command == "position") PositionCommand(iss, board);
            else if (command == "setoption") SetOptionCommand(iss);
            else if (command == "stop") StopCommand();
            else if (command == "ucinewgame") UciNewGameCommand();
            else if (command == "perft") PerftCommand(iss, board);
            else if (command == "board") BoardCommand(board);
            else if (command == "test") TestCommand();
            else if (command == "quit") break;
            else if (command.empty()) continue;
            else UnknownCommand();
        }
    }

    void BoardCommand(const Board &board) {
        std::osyncstream(std::cout) << '\n' << board.PrettyPrint() << std::endl;
    }

    void TestCommand() {
        Testing::RunTests();
    }

    void UnknownCommand() {
        std::osyncstream(std::cout) << "Unknown command, please see the engine documentation for available commands."
                                    << std::endl;
    }

    void UciCommand() {
        std::osyncstream(std::cout)
                << "id name " << PROJECT_NAME << " v. " << PROJECT_VER << '\n'
                << "id author " << PROJECT_AUTHOR << '\n'
                << Options() << '\n'
                << "uciok" << std::endl;
    }

    void GoCommand(std::istringstream &iss, const Board &board) {
        if (Search::run) {
            std::osyncstream(std::cout) << "info Search is already in progress!" << std::endl;
            return;
        }
        Search::Settings settings = ParseSearchOptions(iss);
        Search::StartSearch(settings, board);
    }

    void PerftCommand(std::istringstream &iss, Board &board) {

        Depth depth = 0;
        iss >> depth;

        auto start = Now();
        auto nodes = Testing::Perft<true>(depth, board);
        auto elapsed = ElapsedSince(start);

        std::osyncstream(std::cout) << "\nTime elapsed: " << elapsed << "ms"
                                    << " | Nodes explored: " << nodes
                                    << " | NPS: " << Nps(nodes, elapsed)
                                    << '\n' << std::endl;
    }

    void IsReadyCommand() {
        std::osyncstream(std::cout) << "readyok" << std::endl;
    }

    void PositionCommand(std::istringstream &iss, Board &board) {

        std::string token, fen;
        iss >> token;
        if (token == "startpos") {
            fen = STARTPOS_FEN;
            iss >> token; // "moves"
        } else if (token == "fen") {
            while ((iss >> token) && token != "moves") {
                fen += token + ' ';
            }
        }

        if (!board.NewPosition(fen, Search::chess960)) {
            std::osyncstream(std::cout) << "info Invalid fen: " << fen << std::endl;
            return;
        }

        while (iss >> token) {
            if (!board.MakeUciMove(token)) {
                std::osyncstream(std::cout) << "info Invalid move: " << token << std::endl;
                return;
            }
        }
    }

    void StopCommand() {
        Search::StopSearch();
        Search::WaitFinished(); // wait until search is finished before accepting more commands
    }

    void UciNewGameCommand() {
        if (Search::run) {
            std::osyncstream(std::cout) << "info Search is already in progress!" << std::endl;
            return;
        }
        Search::ClearTT();
    }

    bool IsNumber(std::string_view str) {
        return !str.empty() && std::ranges::all_of(str, ::isdigit);
    }

    bool IsBoolean(std::string_view str) {
        return str == "true" || str == "false";
    }

    void SetOptionCommand(std::istringstream &iss) {

        if (Search::run) {
            std::osyncstream(std::cout) << "info Cannot change settings while search is ongoing!" << std::endl;
            return;
        }

        std::string token, option, value;

        iss >> token >> option;
        while (iss >> token && token != "value") {
            option += ' ' + token;
        }
        iss >> value;

        std::ranges::transform(option, option.begin(), ::tolower);

        if (option == "clear hash") {
            Search::ClearTT();
        } else if (option == "hash" && IsNumber(value)) {
            Search::SetTTSize(std::stoi(value));
        } else if (option == "mute plies" && IsNumber(value)) {
            Search::SetPliesMuted(std::stoi(value));
        } else if (option == "threads" && IsNumber(value)) {
            Search::SetNumThreads(std::stoi(value));
        } else if (option == "multipv" && IsNumber(value)) {
            Search::SetMultiPv(std::stoi(value));
        } else if (option == "move overhead" && IsNumber(value)) {
            Search::SetMoveOverhead(std::stoi(value));
        } else if (option == "uci_showcurrline" && IsBoolean(value)) {
            Search::ShowShowCurrLine(value == "true");
        } else if (option == "show current move" && IsBoolean(value)) {
            Search::ShowCurrMoveInfo(value == "true");
        } else if (option == "ownbook" && IsBoolean(value)) {
            Search::SetUseBook(value == "true");
        } else if (option == "uci_chess960" && IsBoolean(value)) {
            Search::SetChess960(value == "true");
        } else if (option == "send updates frequency" && IsNumber(value)) {
            Search::SetUpdateInterval(std::stoi(value));
        } else {
            std::osyncstream(std::cout)
                    << "info Unknown option or invalid value: " << option << ' ' << value << std::endl;
        }
    }

    Search::Settings ParseSearchOptions(std::istringstream &iss) {

        Search::Settings settings;
        std::string option;

        while (iss >> option) {
            if (option == "wtime") iss >> settings.wtime;
            else if (option == "btime") iss >> settings.btime;
            else if (option == "winc") iss >> settings.winc;
            else if (option == "binc") iss >> settings.binc;
            else if (option == "movestogo") iss >> settings.moves_to_go;
            else if (option == "infinite") settings.infinite = true;
            else if (option == "nodes") {
                iss >> settings.allowed_nodes;
                settings.limit_nodes = true;
            } else if (option == "movetime") {
                iss >> settings.allowed_time;
                settings.limit_time = true;
            } else if (option == "depth") {
                iss >> settings.allowed_depth;
                settings.limit_depth = true;
            } else std::osyncstream(std::cout) << "info Unknown search option: " << option << std::endl;
        }

        return settings;
    }
}
