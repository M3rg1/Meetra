#include "UciHandler.h"
#include "Misc.h"
#include <string>
#include <iostream>
#include <sstream>
#include "MoveGen.h"
#include "Perft.h"
#include "ThreadPool.h"
#include "Search.h"

namespace Meetra {

    std::mutex UciHandler::output_mtx;

    UciHandler::UciHandler() {
        board = Board(STARTPOS_FEN);
        listen = false;
    }

    void UciHandler::Listen() {
        listen = true;
        std::string token;
        std::string input;

        do {
            std::getline(std::cin, input);
            // TODO make lower for sts.GetNextToken - not for the entire string - so we can get the FEN properly capitalized
            // but other shit lowered
            StringTokenStream sts(input, false);
            token = sts.NextToken();

            if (token == "uci") UciCommand();
            else if (token == "isready") IsReadyCommand();
            else if (token == "go") GoCommand(sts);
            else if (token == "position") PositionCommand(sts);
            else if (token == "quit") QuitCommand();
            else if (token == "stop") StopCommand();
            else if (token == "ucinewgame") UciNewGameCommand();
            else if (token == "perft") PerftCommand(sts);

        } while (listen && !std::cin.eof());

        // TODO fix this have some sort of terminating function that waits for everything (also call it in the quit command i guess)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void UciHandler::SendToGui(const std::string &data) {
        std::scoped_lock<std::mutex> lock{output_mtx};
        std::cout << data << std::endl;
    }

    void UciHandler::UciCommand() {
        std::stringstream ss;
        ss << "id name " << GetName() << " v. " << GetVersion() << '\n'
           << "id author " << GetAuthor() << '\n'
           << GetOptions() << '\n'
           << "uciok";
        SendToGui(ss.str());
    }

    void UciHandler::GoCommand(StringTokenStream &sts) {
        if (search.IsSearching()) {
            return;
        }
        ABSearch::SearchSettings settings = InitSearchOptions(sts);
        ThreadPool::PushTask([&, settings]() {
            search.StartSearch(settings);
        });
    }

    void UciHandler::PerftCommand(StringTokenStream &sts) {
        if (sts.HasNext()) {
            Depth depth = std::stoi(sts.NextToken());
            RunPerft(depth, board);
        }
    }

    void UciHandler::IsReadyCommand() {
        SendToGui("readyok");
    }

    void UciHandler::PositionCommand(StringTokenStream &sts) {

        std::string token = sts.NextToken();
        std::string fen;
        if (token == "startpos") {
            fen = STARTPOS_FEN;
            if (sts.HasNext()) token = sts.NextToken();
        } else if (token == "fen") {
            while (sts.HasNext() && ((token = sts.NextToken()) != "moves")) {
                fen.append(token);
                fen.push_back(' ');
            }
        }

        board = Board(fen);

        if (token == "moves") {
            while (sts.HasNext()) {
                MakeUciMove(sts.NextToken());
            }
        }
    }

    void UciHandler::QuitCommand() {
        StopCommand();
        listen = false;
        // await search shutdown
        // TODO will shutdown threadpool as well, and await here in while loop until thread pool running = false
        //  (shutdown = true - static var in destructor set to true)
    }

    void UciHandler::StopCommand() {
        search.StopSearch();
        // should await search completion here maybe?
    }

    void UciHandler::UciNewGameCommand() {
        search.ClearTT();
    }

    void UciHandler::MakeUciMove(const std::string &move_string) {
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

    ABSearch::SearchSettings UciHandler::InitSearchOptions(StringTokenStream &sts) {
        ABSearch::SearchSettings settings;
        settings.max_allowed_depth = DEFAULT_SEARCH_DEPTH;
        settings.allowed_time = DEFAULT_SEARCH_TIME;
        settings.info_to_ui_ms_timer = DEFAULT_UI_SPAM;
        settings.multi_pv = 1;
        settings.board = board;
        ParseSearchOptions(sts, settings);
        return settings;
    }

    void UciHandler::ParseSearchOptions(StringTokenStream &sts, ABSearch::SearchSettings &settings) {
        settings.fixed_timer = false;
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
        }
    }
}
