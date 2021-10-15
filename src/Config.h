#ifndef MEETRA_CONFIG_H
#define MEETRA_CONFIG_H

#include "Defs.h"

constexpr Depth MAX_SEARCH_DEPTH = 128;

constexpr int DEFAULT_SEARCH_THREADS = 1;
constexpr int MAX_SEARCH_THREADS = 32;

inline const std::string BOOK_PATH = "tools/bestmove_r20d20_a5000.mtr.bin";
constexpr int BOOK_DEPTH = 20;

inline const std::string STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MAX_LEGAL_MOVES = 256;
constexpr int MAX_GAME_LENGTH = 1024;

constexpr uint64_t ZOBRIST_SEED = 7299078832781365792;

// search time related consts
constexpr TimeRep DEFAULT_SEARCH_TIME = 60000;
constexpr TimeRep MIN_OVERHEAD = 0;
constexpr TimeRep MAX_OVERHEAD = 10000;
constexpr TimeRep DEFAULT_OVERHEAD = 10;
constexpr TimeRep UPDATE_INFO_INTERVAL = 1000;

// eval consts
constexpr Score POSITIVE_INF = 32000;
constexpr Score NEGATIVE_INF = -32000;
constexpr Score MATE_SCORE = 31000;
constexpr Score DRAW_SCORE = 0;
constexpr Score MIN_MATE_EVAL = MATE_SCORE - MAX_SEARCH_DEPTH;

// TT consts
constexpr size_t MIN_HASH_SIZE = 8; // this should never be 0
constexpr size_t DEFAULT_HASH_SIZE = 128;
constexpr size_t MAX_HASH_SIZE = 8192;
constexpr size_t TT_ENTRIES_PER_BUCKET = 4;

// search stuff
constexpr Score FUTILITY_FACTOR = 90;
constexpr Depth FUTILITY_DEPTH = 6;
constexpr Depth NULL_DEPTH = 4;

// perft testing
constexpr bool ETHEREAL_SUITE = false;
inline const std::string TEST_FILE_PATH = "tools/PerftTests.txt"; // perft960.txt - ethereal suite


#endif //MEETRA_CONFIG_H
