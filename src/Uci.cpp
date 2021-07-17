#include "Uci.h"
#include "Misc.h"
#include <iostream>
#include "MoveGen.h"
#include "Perft.h"
#include "ThreadPool.h"
#include "Search.h"
#include "StringTokenStream.h"
#include <mutex>

namespace Meetra::Uci {

    void UciCommand();
    void IsReadyCommand();
    void GoCommand(StringTokenStream &sts, Board &board, ABSearch &search);
    void UciNewGameCommand(ABSearch &search);
    void PositionCommand(StringTokenStream &sts, Board &board);
    void PerftCommand(StringTokenStream &sts, Board &board);
    void SetOptionCommand(StringTokenStream &sts, ABSearch &search);
    void StopCommand(ABSearch &search);
    void ShowCommand(Board &board);
    void HelpCommand();
    void UnknownCommand();
    void QuitCommand(ABSearch &search);
    ABSearch::SearchSettings InitSearchOptions(StringTokenStream &sts);
    void MakeUciMove(const std::string &move_string, Board &board);
    void ParseSearchOptions(StringTokenStream &sts, ABSearch::SearchSettings &settings);

    std::mutex output_mtx;

    void Listen() {

        Board board;
        ABSearch search;

        std::string token;
        std::string input;

        do {
            std::getline(std::cin, input);
            StringTokenStream sts(input);
            token = sts.NextToken();

            if (token == "uci") UciCommand();
            else if (token == "isready") IsReadyCommand();
            else if (token == "go") GoCommand(sts, board, search);
            else if (token == "position") PositionCommand(sts, board);
            else if (token == "setoption") SetOptionCommand(sts, search);
            else if (token == "stop") StopCommand(search);
            else if (token == "ucinewgame") UciNewGameCommand(search);
            else if (token == "perft") PerftCommand(sts, board);
            else if (token == "show") ShowCommand(board);
            else if (token == "help") HelpCommand();
            else if (token == "quit") QuitCommand(search);
            else { UnknownCommand(); }

        } while (token != "quit" && !std::cin.eof());

        // TODO fix this have some sort of terminating function that waits for everything (also call it in the quit command i guess)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void HelpCommand(){
        SendToGui("This is help.");
    }

    void UnknownCommand(){
        SendToGui("Unknown command, please type 'help' to display available commands.");
    }

    void ShowCommand(Board &board) {
        SendToGui(board.PPBoard());
    }

    void SendToGui(const std::string &data) {
        std::scoped_lock<std::mutex> lock(output_mtx);
        std::cout << data << std::endl;
    }

    void UciCommand() {
        std::string info = "id name " + GetName() + " v. " + GetVersion() + '\n'
                           + "id author " + GetAuthor() + '\n'
                           + GetOptions() + '\n'
                           + "uciok";
        SendToGui(info);
    }

    void GoCommand(StringTokenStream &sts, Board &board, ABSearch &search) {
        if (search.IsSearching()) {
            return;
        }
        ABSearch::SearchSettings settings = InitSearchOptions(sts);
        settings.board = board;
        ThreadPool::PushTask([&, settings]() {
            search.StartSearch(settings);
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
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
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

    void QuitCommand(ABSearch &search) {
        StopCommand(search);
        ThreadPool::Shutdown();
        // await search shutdown
        // TODO will shutdown ThreadPool as well, and await here in while loop until thread pool running = false1
        //  (shutdown = true - static var in destructor set to true)
    }

    void StopCommand(ABSearch &search) {
        search.StopSearch();
        // should await search completion here maybe?
    }

    void UciNewGameCommand(ABSearch &search) {
        search.ClearTT();
    }

    // TODO implement search only certain moves, search only max amount of nodes, etc.
    void SetOptionCommand(StringTokenStream &sts, ABSearch &search) {
        if (sts.NextToken() != "name") return;
        sts.MakeLower();
        std::string option = sts.NextToken();
        if (option == "hash") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                //auto hash_size = std::stoi(sts.NextToken());
                //hash_size = std::clamp(hash_size, MIN_HASH_SIZE, MAX_HASH_SIZE);
                //search.SetTTSize(hash_size);
            }
        } else if (option == "clear") {
            if (sts.HasNext() && sts.NextToken() == "hash") {
                search.ClearTT();
            }
        } else if (option == "multipv") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                auto pv_num = std::stoi(sts.NextToken());
                search.SetMultiPv(pv_num);
            }
        } else if (option == "uci_showcurrline") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                bool show = sts.NextToken() == "true";
                search.ShowShowCurrLine(show);
            }
        } else if (option == "mute") {
            if (sts.HasNext() && sts.NextToken() == "plies" && sts.HasNext() && sts.NextToken() == "value") {
                auto plies_muted = std::stoi(sts.NextToken());
                search.SetPliesMuted(plies_muted);
            }
        } else if (option == "cores") {
            if (sts.HasNext() && sts.NextToken() == "value") {
                auto num_threads = std::stoi(sts.NextToken());
                search.SetNumThreads(num_threads);
            }
        } else if (option == "show") {
            if (sts.HasNext() && sts.NextToken() == "current" && sts.HasNext() && sts.NextToken() == "move" &&
                sts.HasNext() && sts.NextToken() == "value") {
                bool show = sts.NextToken() == "true";
                search.ShowCurrMoveInfo(show);
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

    ABSearch::SearchSettings InitSearchOptions(StringTokenStream &sts) {
        ABSearch::SearchSettings settings;
        ParseSearchOptions(sts, settings);
        return settings;
    }

    void ParseSearchOptions(StringTokenStream &sts, ABSearch::SearchSettings &settings) {
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
    }
}
