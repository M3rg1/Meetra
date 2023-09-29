#include "Uci.h"
#include "Search.h"
#include "TestSuite.h"

#include <Env.h>
#include <iostream>
#include <syncstream>

namespace Uci {

    static std::string Options();
    static void UciCommand();
    static void IsReadyCommand();
    static void GoCommand(std::istringstream &iss, const Board &board);
    static void PositionCommand(std::istringstream &iss, Board &board);
    static void SetOptionCommand(std::istringstream &iss);
    static void StopCommand();
    static void UciNewGameCommand();
    static void PerftCommand(std::istringstream &iss, Board &board);
    static void BoardCommand(const Board &board);
    static void TestCommand();
    static void UnknownCommand();
    static bool IsNumber(std::string_view str);
    static bool IsBoolean(std::string_view str);
    static Search::Settings ParseSearchOptions(std::istringstream &iss);

    void Init() {
        std::ios_base::sync_with_stdio(false); // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=27931
        std::cin.tie(nullptr);
    }

    void Listen() {

        std::cout << PROJECT_NAME << " v. " << PROJECT_VER << "\nMade by " << PROJECT_AUTHOR << '\n' << std::endl;

        Board board;
        std::string input, command;
        while (std::getline(std::cin, input)) {

            std::istringstream iss(std::move(input));
            command.clear();
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

    static std::string Options() {
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

    static void UciCommand() {
        std::osyncstream(std::cout)
                << "id name " << PROJECT_NAME << " v. " << PROJECT_VER << '\n'
                << "id author " << PROJECT_AUTHOR << '\n'
                << Options() << '\n'
                << "uciok" << std::endl;
    }

    static void IsReadyCommand() {
        std::osyncstream(std::cout) << "readyok" << std::endl;
    }

    static void GoCommand(std::istringstream &iss, const Board &board) {
        if (Search::run) {
            std::osyncstream(std::cout) << "info Search is already in progress!" << std::endl;
            return;
        }
        Search::Settings settings = ParseSearchOptions(iss);
        Search::WaitFinished();
        Search::StartSearch(settings, board);
    }

    static void PositionCommand(std::istringstream &iss, Board &board) {

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

    static void SetOptionCommand(std::istringstream &iss) {

        if (Search::run) {
            std::osyncstream(std::cout) << "info Cannot change settings while search is ongoing!" << std::endl;
            return;
        }

        Search::WaitFinished();

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
        } else if (option == "send updates frequency" && IsNumber(value)) {
            Search::SetUpdateInterval(std::stoi(value));
        } else if (option == "uci_showcurrline" && IsBoolean(value)) {
            Search::ShowShowCurrLine(value == "true");
        } else if (option == "show current move" && IsBoolean(value)) {
            Search::ShowCurrMoveInfo(value == "true");
        } else if (option == "ownbook" && IsBoolean(value)) {
            Search::SetUseBook(value == "true");
        } else if (option == "uci_chess960" && IsBoolean(value)) {
            Search::SetChess960(value == "true");
        } else {
            std::osyncstream(std::cout)
                    << "info Unknown option or invalid value: " << option << ' ' << value << std::endl;
        }
    }

    static void StopCommand() {
        Search::StopSearch();
    }

    static void UciNewGameCommand() {
        if (Search::run) {
            std::osyncstream(std::cout) << "info Search is already in progress!" << std::endl;
            return;
        }
        Search::WaitFinished();
        Search::ClearTT();
    }

    static void PerftCommand(std::istringstream &iss, Board &board) {

        Depth depth = 0;
        iss >> depth;

        auto start = Now();
        auto nodes = depth > 0 ? Testing::Perft<true>(depth, board) : 0;
        auto elapsed = ElapsedSince(start);

        std::osyncstream(std::cout) << "\nTime elapsed: " << elapsed << "ms"
                                    << " | Nodes explored: " << nodes
                                    << " | NPS: " << Nps(nodes, elapsed)
                                    << '\n' << std::endl;
    }

    static void BoardCommand(const Board &board) {
        std::osyncstream(std::cout) << '\n' << board << std::endl;
    }

    static void TestCommand() {
        Testing::RunTests();
    }

    static void UnknownCommand() {
        std::osyncstream(std::cout)
                << "Unknown command, please see the engine documentation for available commands." << std::endl;
    }

    static bool IsNumber(std::string_view str) {
        return !str.empty() && std::ranges::all_of(str, ::isdigit);
    }

    static bool IsBoolean(std::string_view str) {
        return str == "true" || str == "false";
    }

    static Search::Settings ParseSearchOptions(std::istringstream &iss) {

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
