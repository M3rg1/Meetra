#include "Uci.h"
#include <iostream>
#include "Perft.h"
#include "ThreadPool.h"
#include "Search.h"
#include "StringTokenStream.h"


namespace Meetra::Uci {

#define LOGO   " __  __         _\n"               \
                "|  \\/  |___ ___| |_ _ _ __ _\n"   \
                "| |\\/| / -_) -_)  _| '_/ _` |\n"  \
                "|_|  |_\\___\\___|\\__|_| \\__,_|"

    std::string GetName() { return "Meetra"; }
    std::string GetVersion() { return "0.0.1"; }
    std::string GetAuthor() { return "M3rg1"; }
    std::string GetOptions() {
        std::stringstream ss;
        ss << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << "\n"
           << "option name Clear Hash type button\n"
           << "option name MultiPV type spin default 1 min 1 max 32\n"
           << "option name UCI_ShowCurrLine type check default false\n"
           << "option name Mute plies type spin default 1 min 1 max 64\n"
           << "option name Cores type spin default " << DEFAULT_SEARCH_THREADS << " min 1 max " << MAX_SEARCH_THREADS << "\n"
           << "option name Show current move type check default true\n"
           << "option name Plies draw type spin default " << DEFAULT_PLY_FOR_DRAW << " min 10 max 100";
        return ss.str();
    }

    void UciCommand();
    void IsReadyCommand();
    void GoCommand(StringTokenStream &sts, Board &board);
    void UciNewGameCommand();
    void PositionCommand(StringTokenStream &sts, Board &board);
    void PerftCommand(StringTokenStream &sts, Board &board);
    void SetOptionCommand(StringTokenStream &sts);
    void StopCommand();
    void ShowCommand(Board &board);
    void HelpCommand();
    void UnknownCommand();
    void MakeUciMove(const std::string &move_string, Board &board);
    Search::SearchSettings ParseSearchOptions(StringTokenStream &sts);

    std::atomic_flag lock;

    void Init() {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }

    void Listen() {

        Board board;

        std::string token;
        std::string input;

        do {
            std::getline(std::cin, input);
            StringTokenStream sts(input);
            token = sts.NextToken();

            if (token == "uci") UciCommand();
            else if (token == "isready") IsReadyCommand();
            else if (token == "go") GoCommand(sts, board);
            else if (token == "position") PositionCommand(sts, board);
            else if (token == "setoption") SetOptionCommand(sts);
            else if (token == "stop") StopCommand();
            else if (token == "ucinewgame") UciNewGameCommand();
            else if (token == "perft") PerftCommand(sts, board);
            else if (token == "show") ShowCommand(board);
            else if (token == "help") HelpCommand();
            else if (token == "quit");
            else { UnknownCommand(); }

        } while (token != "quit" && !std::cin.eof());

        Search::Shutdown();
        ThreadPool::Shutdown();
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

        while (lock.test_and_set(std::memory_order_acquire)) {
            while (lock.test(std::memory_order_relaxed));
        }

        std::cout << data << std::endl;

        lock.clear(std::memory_order_release);
    }

    void UciCommand() {
        std::stringstream ss;
        ss << "id name " << GetName() << " v. " << GetVersion() << '\n'
           << "id author " << GetAuthor() << '\n'
           << GetOptions() << '\n'
           << "uciok";
        SendToGui(ss.str());
    }

    void GoCommand(StringTokenStream &sts, Board &board) {
        if (Search::IsSearching()) {
            return;
        }

        Search::SearchSettings settings = ParseSearchOptions(sts);

        ThreadPool::PushTask([=]() {
            Search::StartSearch(settings, board);
        });
    }

    void PerftCommand(StringTokenStream &sts, Board &board) {
        if (sts.HasNext()) {
            Depth depth = std::stoi(sts.NextToken());
            RunPerft(depth, board);
        }
    }

    void IsReadyCommand() {
        SendToGui("readyok");
    }

    void PositionCommand(StringTokenStream &sts, Board &board) {

        std::string token = sts.NextToken();
        std::string fen;

        if (token == "startpos") {
            fen = STARTPOS_FEN;
            if (sts.HasNext()) token = sts.NextToken();
        } else if (token == "fen") {
            while (sts.HasNext() && ((token = sts.NextToken()) != "moves")) {
                fen += token + ' ';
            }
        }

        board.NewPosition(fen);

        if (token == "moves") {
            while (sts.HasNext()) {
                MakeUciMove(sts.NextToken(), board);
            }
        }
    }

    void StopCommand() {
        Search::StopSearch();
    }

    void UciNewGameCommand() {
        Search::ClearTT();
    }

    void SetOptionCommand(StringTokenStream &sts) {
        if (sts.NextToken() != "name") return;
        sts.MakeLower();
        std::string option = sts.NextToken();
        if (option == "hash") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                int hash_size = std::stoi(sts.NextToken());
                Search::SetTTSize(hash_size);
            }
        } else if (option == "clear") {
            if (sts.HasNext() && sts.NextToken() == "hash") {
                Search::ClearTT();
            }
        } else if (option == "multipv") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                int pv_num = std::stoi(sts.NextToken());
                Search::SetMultiPv(pv_num);
            }
        } else if (option == "uci_showcurrline") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                bool show = sts.NextToken() == "true";
                Search::ShowShowCurrLine(show);
            }
        } else if (option == "mute") {
            if (sts.HasNext() && sts.NextToken() == "plies" && sts.HasNext() && sts.NextToken() == "value") {
                int plies_muted = std::stoi(sts.NextToken());
                Search::SetPliesMuted(plies_muted);
            }
        } else if (option == "cores") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                int num_threads = std::stoi(sts.NextToken());
                Search::SetNumThreads(num_threads);
            }
        } else if (option == "show") {
            if (sts.HasNext() && sts.NextToken() == "current" && sts.HasNext() && sts.NextToken() == "move" &&
                sts.HasNext() && sts.NextToken() == "value") {
                bool show = sts.NextToken() == "true";
                Search::ShowCurrMoveInfo(show);
            }
        } else if (option == "plies") {
            if (sts.HasNext() && sts.NextToken() == "draw" && sts.HasNext() && sts.NextToken() == "value") {
                int plies_draw = std::stoi(sts.NextToken());
                Search::SetPliesDraw(plies_draw);
            }
        }
    }

    void MakeUciMove(const std::string &move_string, Board &board) {
        Move move_made = NewMoveFromName(move_string);
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

    Search::SearchSettings ParseSearchOptions(StringTokenStream &sts) {

        Search::SearchSettings settings;

        while (sts.HasNext()) {
            std::string token = sts.NextToken();
            if (token == "wtime") settings.white_time = std::stoi(sts.NextToken());
            else if (token == "btime") settings.black_time = std::stoi(sts.NextToken());
            else if (token == "winc") settings.white_increment = std::stoi(sts.NextToken());
            else if (token == "binc") settings.black_increment = std::stoi(sts.NextToken());
            else if (token == "movetime") {
                settings.max_allowed_depth = MAX_SEARCH_DEPTH;
                settings.fixed_timer = true;
                settings.allowed_time = std::stoi(sts.NextToken());
            } else if (token == "infinite") {
                settings.max_allowed_depth = MAX_SEARCH_DEPTH;
                settings.infinite = true;
            } else if (token == "depth") {
                settings.max_allowed_depth = std::stoi(sts.NextToken());
                settings.infinite = true;
            }
            //else if (token == "ponder") infinite = true; - need to implement ponderhit command for this (there we set search_timer)
            //else if movestogo - thats when we get time increment
        }

        return settings;
    }
}
