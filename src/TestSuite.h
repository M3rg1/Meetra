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
        Move m;

        if (depth <= 1) {
            while ((m = move_gen.GetAnyMove())) {
                if (board.IsMoveLegal(m)) {
                    ++total_nodes;
                    if constexpr (DIV) {
                        Uci::Send(board.MoveToName(m) + ": " + std::to_string(1));
                    }
                }
            }
            return total_nodes;
        }

        while ((m = move_gen.GetAnyMove())) {
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

    class Test {

        std::string fen;
        Depth depth;
        uint64_t expected;

    public:

        inline bool Run() {

            Board board;
            if (!board.NewPosition(fen)) {
                Uci::Send("Position: " + fen + "\nError parsing FEN, skipping test.\n=== TEST ERROR ===\n");
                return false;
            }

            auto start = Time::Now();

            uint64_t result = Perft<false>(depth, board);

            auto elapsed_ns = Time::ElapsedTime<Time::ns>(start);
            auto elapsed_ms = elapsed_ns / 1000000;
            auto nps = static_cast<uint64_t>(static_cast<double>(result) /
                                             (static_cast<double>(elapsed_ns) / 1000000000.0));

            std::ostringstream oss;
            oss << "Position: " << fen << '\n'
                << "Depth: " << std::to_string(depth) << " | Expected: " << expected <<
                " | Got: " << result << " | Time elapsed: " << elapsed_ms << "ms" << " | NPS: " << nps;

            if (result != expected) {
                oss << "\n=== TEST ERROR ===\n";
            } else {
                oss << "\n=== TEST OK ===\n";
            }

            Uci::Send(oss.str());

            return result == expected;
        }

        friend std::istream &operator>>(std::istream &is, Test &t) {
            std::string token;
            is >> token >> t.depth;
            is >> token >> t.expected;
            is >> token >> t.fen;
            while (is >> token && token != "#") {
                t.fen += ' ' + token;
            }
            return is;
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
            std::istringstream ss(line);
            Test t;
            ss >> t;
            tests.emplace_back(t);
        }

        test_file.close();

        return tests;
    }

    inline void RunTests() {

        auto tests = LoadTests();
        if (tests.empty()) {
            return;
        }

        int errors = 0;
        auto start = Time::Now();

        for (size_t i = 0; i < tests.size(); ++i) {
            Uci::Send("Running test " + std::to_string(i + 1));
            if (!tests[i].Run()) {
                ++errors;
            }
        }

        auto time_elapsed_ms = Time::ElapsedTime<Time::ms>(start);

        Uci::Send("Finished in " + std::to_string(time_elapsed_ms) + "ms.");
        if (errors == 0) {
            Uci::Send("============= ALL TESTS OK =============");
        } else {
            Uci::Send("============= " + std::to_string(errors) + " ERRORS IN TESTS =============");
        }
    }

}

#endif //MEETRA_TESTSUITE_H
