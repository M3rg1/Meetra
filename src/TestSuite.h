#ifndef MEETRA_TESTSUITE_H
#define MEETRA_TESTSUITE_H

#include <string>
#include <vector>
#include "Defs.h"
#include "Board.h"
#include "MoveGen.h"
#include "Uci.h"
#include <fstream>
#include <sstream>
#include "Time.h"
#include "Config.h"

namespace Testing {

    template<bool DIV>
    uint64_t Perft(Depth depth, Board &board) {

        MoveGen move_gen(board);
        uint64_t total_nodes = 0;

        if (depth <= 1) {
            while (Move m = move_gen.GetAnyMove()) {
                if (board.IsMoveLegal(m)) {
                    ++total_nodes;
                    if constexpr (DIV) {
                        Uci::Send(board.MoveToName(m) + ": " + std::to_string(1));
                    }
                }
            }
            return total_nodes;
        }

        while (Move m = move_gen.GetAnyMove()) {
            if (!board.MakeMove(m)) {
                board.UnmakeMove(m);
                continue;
            }
            uint64_t nodes = Perft<false>(depth - 1, board);
            board.UnmakeMove(m);
            total_nodes += nodes;
            if constexpr (DIV) {
                Uci::Send(board.MoveToName(m) + ": " + std::to_string(nodes));
            }
        }

        return total_nodes;
    }

    struct Test {

        std::string fen;
        Depth depth = 0;
        uint64_t expected = 0;
        uint64_t result = 0;
        Board board;

        explicit Test(const std::string &str) {

            std::istringstream iss(str);
            std::string token;

            if (str.contains("FRC")) {
                board.SetChess960(true);
                iss >> token;
            }

            iss >> token >> depth;
            iss >> token >> expected;
            iss >> token >> fen;
            while (iss >> token && token != "#") {
                fen += ' ' + token;
            }
        }

        inline bool Run() {

            if (!board.NewPosition(fen)) {
                Uci::Send("Position: " + fen + "\nError parsing FEN, skipping test.\n=== ERROR ===\n");
                return false;
            }

            auto start = Time::Now();

            result = Perft<false>(depth, board);

            auto elapsed_ns = Time::ElapsedTime<Time::ns>(start) + 1;
            auto elapsed_ms = elapsed_ns / 1000000;
            auto nps = static_cast<uint64_t>((static_cast<double>(result) / static_cast<double>(elapsed_ns))
                                             * 1000000000.0);

            std::ostringstream oss;
            oss << "Position: " << fen << '\n'
                << "Depth: " << std::to_string(depth)
                << " | Expected: " << expected
                << " | Got: " << result
                << " | Time elapsed: " << elapsed_ms << "ms"
                << " | NPS: " << nps << '\n'
                << (result != expected ? "=== ERROR ===" : "=== OK ===") << '\n';

            Uci::Send(oss.str());

            return result == expected;
        }
    };

    inline std::vector<Test> LoadTests() {

        std::fstream test_file;
        test_file.open(TEST_FILE_PATH, std::ios::in);
        if (!test_file.is_open()) {
            Uci::Send("Could not open the test file!");
            return {};
        }

        std::vector<Test> tests;
        std::string line;

        while (getline(test_file, line)) {
            if (line.empty() || line[line.find_first_not_of(' ')] == '#') {
                continue;
            }
            tests.emplace_back(line);
        }

        return tests;
    }

    inline void RunTests() {

        auto tests = LoadTests();
        if (tests.empty()) {
            return;
        }

        int errors = 0;
        uint64_t total_nodes = 0;
        auto start = Time::Now();

        for (size_t i = 0; i < tests.size(); ++i) {
            Uci::Send("Running test " + std::to_string(i + 1) + " ...");
            if (!tests[i].Run()) {
                ++errors;
            }
            total_nodes += tests[i].result;
        }

        auto elapsed_ns = Time::ElapsedTime<Time::ns>(start) + 1;
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(total_nodes) / static_cast<double>(elapsed_ns))
                                         * 1000000000.0);

        std::ostringstream oss;
        oss << "==========================================\n\n"
            << "Total time elapsed: " << elapsed_ms << "ms\n"
            << "NPS: " + std::to_string(nps) + "\n"
            << "Errors found: " + std::to_string(errors);

        Uci::Send(oss.str());
    }

}

#endif //MEETRA_TESTSUITE_H
