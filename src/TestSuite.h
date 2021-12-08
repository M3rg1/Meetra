#ifndef MEETRA_TESTSUITE_H
#define MEETRA_TESTSUITE_H

#include <string>
#include <vector>
#include "Defs.h"
#include "Board.h"
#include "MoveGen.h"
#include <fstream>
#include <sstream>
#include "Timer.h"
#include "Config.h"
#include <syncstream>
#include <iostream>

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
                        std::osyncstream(std::cout) << board.MoveToName(m) << ": 1" << std::endl;
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
                std::osyncstream(std::cout) << board.MoveToName(m) << ": " << nodes << std::endl;
            }
        }

        return total_nodes;
    }

    struct Test {

        std::string fen;
        Depth depth = 0;
        uint64_t expected = 0;
        uint64_t result = 0;
        bool chess960 = false;

        explicit Test(const std::string &str) {

            std::istringstream iss(str);
            std::string token;

            iss >> token;
            if (token == "FRC") {
                chess960 = true;
                iss >> token; // depth
            }
            iss >> depth;
            iss >> token >> expected;
            iss >> token >> fen;
            while (iss >> token && !token.starts_with('#')) {
                fen += ' ' + token;
            }
        }

        inline bool Run() {

            Board board;
            if (!board.NewPosition(fen, chess960)) {
                std::osyncstream(std::cout)
                        << "Position: " << fen << "\nError parsing FEN, skipping test.\n=== ERROR ===\n" << std::endl;
                return false;
            }

            auto start = Time::Now();

            result = Perft<false>(depth, board);

            auto elapsed_ns = Time::ElapsedSince<Time::ns>(start) + 1;
            auto elapsed_ms = elapsed_ns / 1000000;
            auto nps = static_cast<uint64_t>((static_cast<double>(result) / static_cast<double>(elapsed_ns))
                                             * 1000000000.0);

            std::osyncstream(std::cout)
                    << "Position: " << fen << '\n'
                    << "Depth: " << depth
                    << " | Expected: " << expected
                    << " | Got: " << result
                    << " | Time elapsed: " << elapsed_ms << "ms"
                    << " | NPS: " << nps << '\n'
                    << (result != expected ? "=== ERROR ===" : "=== OK ===") << '\n' << std::endl;

            return result == expected;
        }
    };

    inline std::vector<Test> LoadTests() {

        std::fstream test_file(TEST_FILE_PATH, std::ios::in);
        if (!test_file.is_open()) {
            std::osyncstream(std::cout) << "Could not open the test file!" << std::endl;
            return {};
        }

        std::vector<Test> tests;
        std::string line;

        while (getline(test_file, line)) {
            if (std::ranges::all_of(line, ::isspace) || line[line.find_first_not_of(' ')] == '#') {
                continue;
            }
            tests.emplace_back(line);
        }

        return tests;
    }

    inline void RunTests() {

        auto tests = LoadTests();
        if (tests.empty()) {
            std::osyncstream(std::cout) << "No tests to run." << std::endl;
            return;
        }

        std::vector<int> errors;
        uint64_t nodes = 0;
        auto start = Time::Now();

        for (size_t i = 0; i < tests.size(); ++i) {
            std::osyncstream(std::cout) << "Running test " << i + 1 << " ..." << std::endl;
            if (!tests[i].Run()) {
                errors.emplace_back(i);
            }
            nodes += tests[i].result;
        }

        auto elapsed_ns = Time::ElapsedSince<Time::ns>(start) + 1;
        auto elapsed_ms = elapsed_ns / 1000000;
        auto nps = static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns)) * 1000000000.0);

        std::osyncstream oss(std::cout);
        oss << "==========================================\n\n"
            << "Total time elapsed: " << elapsed_ms << "ms\n"
            << "Average NPS: " << nps << '\n';
        if (!errors.empty()) {
            oss << "Errors found in tests:\n";
            for (auto e: errors) {
                oss << (e + 1) << ". FEN: " << tests[e].fen << '\n';
            }
            oss << "========\nTotal errors: " << errors.size();
        } else {
            oss << "All tests OK";
        }
        oss << std::endl;
    }

}

#endif //MEETRA_TESTSUITE_H
