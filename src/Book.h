#ifndef MEETRA_BOOK_H
#define MEETRA_BOOK_H

#include <vector>
#include "ZobristHash.h"
#include "Board.h"
#include <filesystem>
#include <fstream>
#include "Defs.h"
#include "Config.h"
#include <iostream>
#include <syncstream>

namespace Book {

    struct BookEntry {
        uint64_t hash;
        uint16_t move;
    };

    inline std::vector<Move> BinarySearch(std::ifstream &stream, size_t size_bytes, Hash64 hash) {

        std::vector<Move> moves;
        std::streamoff right = static_cast<std::streamoff>(size_bytes / sizeof(BookEntry));
        std::streamoff left = 0;
        BookEntry book_entry;

        while (left <= right) {

            std::streamoff mid = (right + left) / 2;
            stream.seekg(static_cast<std::streamoff>(mid * sizeof(BookEntry)), std::ios::beg);
            stream.read(reinterpret_cast<char *>(&book_entry), sizeof(BookEntry));

            if(!stream.good()) {
                break;
            }

            if (book_entry.hash > hash) {
                right = mid - 1;
            } else if (book_entry.hash < hash) {
                left = mid + 1;
            } else {

                do {
                    moves.emplace_back(book_entry.move);
                } while (stream.read(reinterpret_cast<char *>(&book_entry), sizeof(BookEntry))
                         && book_entry.hash == hash);

                int i = 1;
                while (stream.seekg(static_cast<std::streamoff>((mid - i) * sizeof(BookEntry)), std::ios::beg)
                       && stream.read(reinterpret_cast<char *>(&book_entry), sizeof(BookEntry))
                       && book_entry.hash == hash
                        ) {
                    moves.emplace_back(book_entry.move);
                    ++i;
                }

                break;
            }
        }

        return moves;
    }

    inline std::vector<Move> Probe(Hash64 hash) {

        std::ifstream book_stream(BOOK_PATH, std::ios::in | std::ios::binary);
        if (!book_stream.is_open()) {
            std::osyncstream(std::cout) << "info Could not open book." << std::endl;
            return {};
        }

        size_t size = std::filesystem::file_size(BOOK_PATH);
        return BinarySearch(book_stream, size, hash);
    }
}

#endif //MEETRA_BOOK_H
