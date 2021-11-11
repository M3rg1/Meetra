#include <iostream>
#include "Uci.h"
#include "Search.h"
#include "TestSuite.h"
#include <unistd.h>

namespace Uci {

    const std::string LOGO = " __  __         _\n"               \
                             "|  \\/  |___ ___| |_ _ _ __ _\n"   \
                             "| |\\/| / -_) -_)  _| '_/ _` |\n"  \
                             "|_|  |_\\___\\___|\\__|_| \\__,_|";

    const std::string NAME = "Meetra";
    const std::string VERSION = "0.0.1";
    const std::string AUTHOR = "M3rg1";

    std::string GetOptions() {
        std::ostringstream oss;
        oss << "option name Clear Hash type button\n"
            << "option name UCI_ShowCurrLine type check default false\n"
            << "option name Show current move type check default true\n"
            << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE
            << " max " << MAX_HASH_SIZE << "\n"
            << "option name MultiPV type spin default 1 min 1 max 32\n"
            << "option name Mute plies type spin default 0 min 0 max 64\n"
            << "option name OwnBook type check default false\n"
            << "option name Threads type spin default " << DEFAULT_SEARCH_THREADS << " min 1 max "
            << MAX_SEARCH_THREADS << "\n"
            << "option name Move overhead type spin default " << DEFAULT_OVERHEAD << " min "
            << MIN_OVERHEAD << " max " << MAX_OVERHEAD << "\n"
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
    Search::Settings ParseSearchOptions(std::istringstream &iss);

    void Init() {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }

    void Send(std::string_view data) {

        static std::mutex mtx;
        std::scoped_lock lock(mtx);

        std::cout << data << std::endl;
    }

    void SendInfo(const std::string &data) {
        Send("info string " + data);
    }

    void Listen() {

        Board board;

        if (isatty(STDOUT_FILENO)) {
            Send(LOGO + "\n"
                 + " v. " + VERSION + "\n"
                 + " Made by " + AUTHOR + "\n\n"
                 + board.PrettyPrint());
        }

        std::string command, input;

        do {

            command.clear();
            std::getline(std::cin, input);
            std::istringstream iss(input);
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
            else if (command == "quit") QuitCommand();
            else if (command.empty()) continue;
            else UnknownCommand();

        } while (command != "quit" && !std::cin.eof());
    }

    void BoardCommand(const Board &board) {
        Uci::Send('\n' + board.PrettyPrint());
    }

    void TestCommand() {
        Testing::RunTests();
    }

    void QuitCommand() {
        Search::Shutdown();
    }

    void UnknownCommand() {
        Send("Unknown command, please see the engine documentation for available commands.");
    }

    void UciCommand() {
        Send(
                "id name " + NAME + " v. " + VERSION + '\n'
                + "id author " + AUTHOR + '\n'
                + GetOptions() + '\n'
                + "uciok"
        );
    }

    void GoCommand(std::istringstream &iss, const Board &board) {
        if (Search::run) {
            SendInfo("Search is already in progress!");
            return;
        }
        Search::Settings settings = ParseSearchOptions(iss);
        Search::StartSearch(settings, board);
    }

    void PerftCommand(std::istringstream &iss, Board &board) {

        Depth depth = 0;
        iss >> depth;

        auto start = Time::Now();

        auto nodes = Testing::Perft<true>(depth, board);

        auto elapsed_ns = Time::ElapsedTime<Time::ns>(start) + 1;
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);

        std::ostringstream oss;
        oss << "\nTime elapsed: " << elapsed_ms << "ms"
            << " | Nodes explored: " << nodes
            << " | NPS: " << nps;

        Send(oss.str());
    }

    void IsReadyCommand() {
        Send("readyok");
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
            SendInfo("Invalid fen: " + fen);
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
        Search::WaitFinished(); // wait until search is finished before accepting more commands
    }

    void UciNewGameCommand() {
        if (Search::run) {
            SendInfo("Search is already in progress!");
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
            SendInfo("Cannot change settings while search is ongoing!");
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
            Search::SetTTSize(std::stoull(value));
        } else if (option == "mute plies" && IsNumber(value)) {
            Search::SetPliesMuted(std::stoi(value));
        } else if (option == "threads" && IsNumber(value)) {
            Search::SetNumThreads(std::stoull(value));
        } else if (option == "multipv" && IsNumber(value)) {
            Search::SetMultiPv(std::stoull(value));
        } else if (option == "move overhead" && IsNumber(value)) {
            Search::SetMoveOverhead(std::stoll(value));
        } else if (option == "uci_showcurrline" && IsBoolean(value)) {
            Search::ShowShowCurrLine(value == "true");
        } else if (option == "show current move" && IsBoolean(value)) {
            Search::ShowCurrMoveInfo(value == "true");
        } else if (option == "ownbook" && IsBoolean(value)) {
            Search::SetUseBook(value == "true");
        } else if (option == "uci_chess960" && IsBoolean(value)) {
            Search::SetChess960(value == "true");
        } else {
            SendInfo("Unknown option or invalid value: " + option + ' ' + value);
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
            else if (option == "infinite") {
                settings.fixed = true;
            } else if (option == "nodes") {
                iss >> settings.allowed_nodes;
                settings.fixed = true;
            } else if (option == "movetime") {
                iss >> settings.allowed_time;
                settings.fixed = true;
            } else if (option == "depth") {
                iss >> settings.allowed_depth;
                settings.fixed = true;
            } else {
                SendInfo("Unknown search option: " + option);
            }
        }

        return settings;
    }
}
