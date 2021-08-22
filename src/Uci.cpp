#include <iostream>
#include "Uci.h"
#include "Perft.h"
#include "Search.h"
#include "Spinlock.h"

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
            << "option name Cores type spin default " << DEFAULT_SEARCH_THREADS << " min 1 max " << MAX_SEARCH_THREADS
            << "\n"
            << "option name Plies draw type spin default " << DEFAULT_PLY_FOR_DRAW << " min 10 max 150";
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
    void ShowCommand(Board &board);
    void HelpCommand();
    void PBoardCommand(Board &b);
    void QuitCommand();
    void UnknownCommand();
    void MakeUciMove(const std::string &move_string, Board &board);
    Search::SearchSettings ParseSearchOptions(std::istringstream &iss);

    void Init() {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }

    void Listen() {

        Board board;

        if (isatty(STDOUT_FILENO)) {
            SendToGui(GetLogo() + "\n"
                      + " v. " + GetVersion() + "\n"
                      + " Made by " + GetAuthor() + "\n\n"
                      + board.PPBoard());
        }

        std::string token;
        std::string input;

        do {
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
            else if (token == "show") ShowCommand(board);
            else if (token == "help") HelpCommand();
            else if (token == "pboard") PBoardCommand(board);
            else if (token == "quit") QuitCommand();
            else { UnknownCommand(); }

        } while (token != "quit" && !std::cin.eof());

    }

    void PBoardCommand(Board &b) {
        Uci::SendToGui(b.PPBoard());
    }

    void QuitCommand() {
        Search::Shutdown();
    }

    void HelpCommand() {
        SendToGui("This is help.");
    }

    void UnknownCommand() {
        SendToGui("Unknown command, type 'help' to display available commands.");
    }

    void ShowCommand(Board &board) {
        SendToGui(board.PPBoard());
    }

    void SendToGui(const std::string &data) {

        static Spinlock spinlock;
        ScopedSpinlock lock(spinlock);

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
            iss >> token;
        } else if (token == "fen") {
            while ((iss >> token) && token != "moves") {
                fen += token + ' ';
            }
        }

        board.NewPosition(fen);

        if (token == "moves") {
            while (iss >> token) {
                MakeUciMove(token, board);
            }
        }
    }

    void StopCommand() {
        Search::StopSearch();
        while (!Search::Finished());
    }

    void UciNewGameCommand() {
        Search::ClearTT();
    }

    void SetOptionCommand(std::istringstream &iss) {

        std::string token;
        iss >> token;
        if (!Search::Finished() || token != "name") {
            return;
        }

        std::string option;
        while (iss >> token && token != "value") {
            option += token + " ";
        }

        std::string value;
        iss >> value;

        option.erase(option.find_last_not_of(' ') + 1);
        std::transform(option.begin(), option.end(), option.begin(), ::tolower);

        if (option == "hash") {
            int hash_size = std::stoi(value);
            Search::SetTTSize(hash_size);
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
        } else if (option == "plies draw") {
            Search::SetPliesDraw(std::stoi(value));
        }
    }

    void MakeUciMove(const std::string &move_string, Board &board) {
        Move move_made = NewMoveFromName(move_string);
        MoveGen move_gen(board);
        Move move;
        while ((move = move_gen.GetAnyMove())) {
            if (FromSquare(move) == FromSquare(move_made) && ToSquare(move) == ToSquare(move_made)) {
                if (IsPromotion(move) && GetMoveType(move) != GetMoveType(move_made)) {
                    continue;
                }
                board.MakeMove(move);
                break;
            }
        }
    }

    Search::SearchSettings ParseSearchOptions(std::istringstream &iss) {

        Search::SearchSettings settings;
        std::string option;
        std::string value;

        while (iss >> option) {
            iss >> value;
            if (option == "wtime") settings.white_time = std::stoi(value);
            else if (option == "btime") settings.black_time = std::stoi(value);
            else if (option == "winc") settings.white_increment = std::stoi(value);
            else if (option == "binc") settings.black_increment = std::stoi(value);
            else if (option == "movetime") {
                settings.max_allowed_depth = MAX_SEARCH_DEPTH;
                settings.fixed_timer = true;
                settings.allowed_time = std::stoi(value);
            } else if (option == "infinite") {
                settings.max_allowed_depth = MAX_SEARCH_DEPTH;
                settings.infinite = true;
            } else if (option == "depth") {
                settings.max_allowed_depth = std::stoi(value);
                settings.infinite = true;
            }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
            //else if movestogo - thats when we get time increment
        }

        return settings;
    }
}
