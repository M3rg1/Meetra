#include "Book.h"
#include <filesystem>
#include <fstream>
#include "Uci.h"
#include "MoveGen.h"
#include <algorithm>

namespace Meetra::Book {

#define POS_REPEATED 5
#define FILE_NAME "tools/bestmove_r5d30.mtr.bin"
#define MAX_DEPTH 30
#define INPUT_FILE "all.pgn"

    struct BookEntry_count {
        Hash64 hash;
        Move move;
        int count = 0;
    };

    std::vector<Move> BinarySearch(std::ifstream &stream, size_t left, size_t right, Hash64 hash) {

        std::vector<Move> moves;
        stream.seekg(0, std::ios::beg);
        right /= sizeof(BookEntry);
        BookEntry book_entry;

        while (left <= right) {

            auto mid = (right + left) / 2;
            stream.seekg(mid * sizeof(BookEntry), std::ios::beg);

            stream.read((char *) &book_entry, sizeof(BookEntry));

            if (book_entry.hash > hash) {
                right = mid - 1;
                continue;
            }

            if (book_entry.hash < hash) {
                left = mid + 1;
                continue;
            }

            if (book_entry.hash == hash) {
                do {
                    moves.emplace_back(book_entry.move);
                } while (stream.read((char *) &book_entry, sizeof(BookEntry)) && book_entry.hash == hash);

                size_t i = 1;
                while (stream.seekg((mid - i) * sizeof(BookEntry), std::ios::beg) &&
                       stream.read((char *) &book_entry, sizeof(BookEntry)) && book_entry.hash == hash) {
                    moves.emplace_back(book_entry.move);
                    i++;
                }
                break;
            }
        }

        return moves;
    }

    std::vector<Move> ProbeBook(const Board &board) {
        std::ifstream read_book;
        read_book.open(FILE_NAME, std::ios::in | std::ios::binary);
        if (!read_book.is_open()) {
            Uci::SendInfo("Could not open book.");
            return {};
        }

        size_t end = std::filesystem::file_size(FILE_NAME);
        return BinarySearch(read_book, 0, end, board.GetHash());
    }


    bool SaveBook(const std::vector<BookEntry> &book_entries) {

        std::ofstream book_file;
        book_file.open(FILE_NAME, std::ios::out | std::ios::binary);
        if (!book_file.is_open()) {
            return false;
        }

        for (const auto &e: book_entries) {
            book_file.write((char *) &e, sizeof(BookEntry));
        }

        return true;
    }


    std::vector<BookEntry> RemoveBadPositions(std::vector<BookEntry_count> &positions) {

        auto cmp_pos = [](const auto &e1, const auto &e2) {
            return e1.hash != e2.hash ? e1.hash < e2.hash : e1.move < e2.move;
        };

        std::ranges::sort(positions, cmp_pos);

        std::vector<BookEntry_count> out;
        int repeats = 0;
        for (int i = 0; i < positions.size() - 1; i++) {
            if (positions[i].hash == positions[i + 1].hash && positions[i].move == positions[i + 1].move) {
                repeats++;
            } else {
                if (repeats >= POS_REPEATED) {
                    positions[i].count = repeats;
                    out.emplace_back(positions[i]);
                }
                repeats = 0;
            }
        }

        std::ranges::sort(out, cmp_pos);

        std::vector<BookEntry> final;

        for (int i = 0; i < out.size() - 1; i++) {
            BookEntry_count best = out[i];
            while (i < out.size() - 1 && out[i].hash == out[i + 1].hash) {
                if (best.count <= out[i + 1].count) {
                    if (best.count > 10000) {
                        final.emplace_back(BookEntry{best.hash, best.move});
                    }
                    best = out[i + 1];
                }
                i++;
            }
            final.emplace_back(BookEntry{best.hash, best.move});
        }

        Uci::Send("Valid positions to save: " + std::to_string(final.size()));

        return final;
    }


    std::vector<BookEntry_count> ParsePgn() {

        std::ifstream pgn_file;

        pgn_file.open(INPUT_FILE, std::ios::in);
        if (!pgn_file.is_open()) {
            return {};
        }

        std::vector<BookEntry_count> positions;

        std::string_view pieces = "PKQRBN";
        std::string_view files = "abcdefgh";
        std::string_view ranks = "12345678";

        size_t move_n = 0;
        size_t moves_cnt = 0;

        std::string line;
        Board board;

        while (getline(pgn_file, line)) {

            // comment or empty line
            if (line.empty() || line.starts_with('[')) {
                continue;
            }

            // new game
            if (line.starts_with("1.")) {
                board.NewPosition(STARTPOS_FEN);
                move_n = 0;
            }

            if (move_n >= MAX_DEPTH) {
                continue;
            }

            std::istringstream ss(line);
            std::string token;
            while (ss >> token) {

                if (move_n >= MAX_DEPTH || token == ("1-0") || token == "0-1" || token == "1/2-1/2" || token == "*") {
                    break;
                }

                std::string origin;
                std::string destination;
                char promotion_to = 0;
                char piece_str = 0;

                Color col_move = move_n % 2 == 0 ? WHITE : BLACK;

                // remove the move number from the beginning
                if (token.find('.') != std::string::npos) {
                    token.erase(0, token.find('.') + 1);
                }

                // remove check or checkmate symbol from the end
                if (token.find('+') != std::string::npos || token.find('#') != std::string::npos) {
                    token.erase(token.length() - 1);
                }

                // castle short
                if (token == "O-O") {
                    piece_str = 'K';
                    origin = col_move == WHITE ? "e1" : "e8";
                    destination = col_move == WHITE ? "g1" : "g8";
                    token.clear();
                }

                // castle long
                if (token == "O-O-O") {
                    piece_str = 'K';
                    origin = col_move == WHITE ? "e1" : "e8";
                    destination = col_move == WHITE ? "c1" : "c8";
                    token.clear();
                }

                // remove the take symbol
                if (token.find('x') != std::string::npos) {
                    token.erase(token.find('x'), 1);
                }

                // promotion move
                if (token.find('=') != std::string::npos) {
                    promotion_to = token[token.find('=') + 1];
                    token.erase(token.find('='), 2);
                }

                // after trimming the string, now last 2 chars are destination square
                if (!token.empty()) {
                    destination = token.substr(token.length() - 2, 2);
                    token.erase(token.length() - 2);
                }

                // the moved piece_str is now guaranteed to be on the first place of the token
                if (!token.empty() && pieces.find(token[0]) != std::string::npos) {
                    piece_str = token[0];
                    token.erase(0, 1);
                    // if no explicit piece symbol, it's a pawn move (just gotta check in case it's a castling move)
                } else if (piece_str != 'K') {
                    piece_str = 'P';
                }

                // if it's an ambiguous move, there will be an origin rank specified
                if (!token.empty() && files.find(token[0]) != std::string::npos) {
                    origin += token[0];
                    token.erase(0, 1);
                }

                // if there wasn't origin rank, or the rank wasn't enough, there will be file rank
                if (!token.empty() && ranks.find(token[0]) != std::string::npos) {
                    origin += token[0];
                }

                // all pieces are uppercase in PGN notation, we need to convert to lowercase if it's a black piece
                if (col_move == BLACK) {
                    piece_str = static_cast<char>(std::tolower(piece_str));
                }

                MoveType flag = NO_FLAG;
                if (promotion_to) {
                    flag = promotion_to == 'Q' ? PROMOTE_QUEEN :
                           promotion_to == 'R' ? PROMOTE_ROOK :
                           promotion_to == 'B' ? PROMOTE_BISHOP :
                           PROMOTE_KNIGHT;
                }

                Square to = NameToSquare(destination);
                Piece piece = CharToPiece(piece_str);

                bool move_ok = false;

                MoveGen move_gen(board);
                Move move;
                while ((move = move_gen.GetAnyMove())) {

                    // destination square is correct
                    if (to != ToSquare(move)) {
                        continue;
                    }

                    Square from = FromSquare(move);

                    // moved piece is on the from square
                    if (board.GetPieceOnSquare(from) != piece) {
                        continue;
                    }

                    // we have full square name from the pgn, make sure it matches
                    if (origin.length() == 2 && from != NameToSquare(origin)) {
                        continue;
                    }
                    // we have either file or rank in the origin string
                    if (origin.length() == 1 && !(FileFromChar(origin[0]) == FileFromSquare(from) ||
                                                  RankFromChar(origin[0]) == RankFromSquare(from))) {
                        continue;
                    }
                    // promotion move, make sure it has the correct promotion flag
                    if (IsPromotion(move) && flag != GetMoveType(move)) {
                        continue;
                    }

                    positions.emplace_back(BookEntry_count{board.GetHash(), move});
                    move_ok = true;
                    if (!board.MakeMove(move)) {
                        Uci::Send("This should not happen! Line: " + line);
                    }
                    break;
                }

                if (!move_ok) {
                    Uci::Send("ERROR - line: " + line);
                }

                moves_cnt++;
                move_n++;

                if (moves_cnt % 1000000 == 0) {
                    Uci::Send("Moves done: " + std::to_string(moves_cnt));
                }
            }
        }

        Uci::Send("DONE LOADING PGN - Positions found: " + std::to_string(moves_cnt));

        return positions;
    }

    void CreateBook() {

        auto entries = ParsePgn();
        if (entries.empty()) {
            Uci::Send("No entries loaded from PGN file.");
            return;
        }

        auto cleaned_entries = RemoveBadPositions(entries);
        if (cleaned_entries.empty()) {
            Uci::Send("No valid entries to save.");
            return;
        }

        if (!SaveBook(cleaned_entries)) {
            Uci::Send("Err saving book");
        }
    }
}
