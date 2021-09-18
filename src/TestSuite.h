#ifndef MEETRA_TESTSUITE_H
#define MEETRA_TESTSUITE_H

#include <string>
#include <vector>
#include "Types.h"
#include "Board.h"
#include "MoveGen.h"
#include "Uci.h"
#include <fstream>
#include <sstream>
#include <chrono>

namespace Meetra::TestSuite {

#define TEST_FILE_PATH "tools/PerftTests.txt"


    class Test {

    private :
        std::string fen;
        Depth depth = 0;
        uint64_t expected = 0;

    public:
        inline bool Run() {

            Board board;
            if (!board.NewPosition(fen)) {
                Uci::Send("Error parsing FEN, skipping test.\n=== TEST ERROR ===\n");
                return false;
            }

            auto start = std::chrono::steady_clock::now();

            uint64_t result = RunPerft(depth, board);

            auto end = std::chrono::steady_clock::now();
            auto time_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() + 1;
            auto nps = static_cast<uint64_t> (static_cast<double>(result) /
                                              (static_cast<double>(time_elapsed_ns) / 1000000000.0));
            auto time_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::ostringstream oss;
            oss << "Position: " << fen << '\n'
                << "Depth: " << std::to_string(depth) << " | Expected: " << expected <<
                " | Got: " << result << " | Time elapsed: " << time_elapsed_ms << "ms" << " | NPS: " << nps;

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
            // for parsing Ethereal's chess 960 perft suite
/*            while(is >> token && token != ";D1") {
                t.fen += token + " ";
            }
            t.fen.pop_back();
            while(is >> token && token != ";D5");
            t.depth = 5;
            is >> t.expected;*/

            if (is >> token && token == "depth") {
                is >> t.depth;
            }
            if (is >> token && token == "result") {
                is >> t.expected;
            }
            if (is >> token && token == "fen") {
                while (is >> token && token != "#") {
                    t.fen += token + " ";
                }
                t.fen.pop_back();
            }
            return is;
        }
    };

    inline std::vector<Test> LoadTests() {
        std::vector<Test> tests;
        std::fstream test_file;
        test_file.open(TEST_FILE_PATH, std::ios::in);
        if (test_file.is_open()) {
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
        } else {
            Uci::SendInfo("Could not open the 'PerftTests.txt' file!");
        }
        return tests;
    }

    inline void RunTests() {

        auto tests = LoadTests();
        if(tests.empty()) {
            return;
        }

        auto start = std::chrono::steady_clock::now();

        int errors = 0;
        for (size_t i = 0; i < tests.size(); i++) {
            Uci::Send("Running test " + std::to_string(i + 1));
            if (!tests[i].Run()) {
                errors++;
            }
        }

        auto end = std::chrono::steady_clock::now();
        auto time_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        Uci::Send("Finished in " + std::to_string(time_elapsed_ms) + "ms.");

        if (errors == 0) {
            Uci::Send("============= ALL TESTS OK =============");
        } else {
            Uci::Send("============= " + std::to_string(errors) + " ERRORS IN TESTS =============");
        }
    }

}

#endif //MEETRA_TESTSUITE_H
