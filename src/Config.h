#ifndef MEETRA_CONFIG_H
#define MEETRA_CONFIG_H

#include "Defs.h"

constexpr Depth BOOK_DEPTH = 20; // TODO this should be read from the book itself and put in var in Book.h
inline const std::string BOOK_PATH = "books/bestmove_r20d20_a5000.mtr.bin"; // TODO this should be UCI setting (var in Book.h)
inline const std::string TEST_FILE_PATH = "tests/PerftTests.txt";

#pragma region ===== Global limits =====
constexpr size_t MAX_LEGAL_MOVES = 256;
constexpr size_t MAX_GAME_LENGTH = 1024;
constexpr Depth MAX_SEARCH_DEPTH = 128;
inline const std::string STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
#pragma endregion

#pragma region ===== Various UCI settings limits =====
constexpr bool DEFAULT_CHESS960 = false;
constexpr bool DEFAULT_USE_BOOK = false;
constexpr bool DEFAULT_SHOW_CURRLINE = false;
constexpr bool DEFAULT_SHOW_CURRMOVE = true;

constexpr TimeRep DEFAULT_OVERHEAD = 10;
constexpr TimeRep MIN_OVERHEAD = 0;
constexpr TimeRep MAX_OVERHEAD = 10000;

constexpr TimeRep DEFAULT_UPDATE_INTERVAL = 1000;
constexpr TimeRep MIN_UPDATE_INTERVAL = 10;
constexpr TimeRep MAX_UPDATE_INTERVAL = 1000000000;

constexpr size_t DEFAULT_SEARCH_THREADS = 1;
constexpr size_t MIN_SEARCH_THREADS = 1;
constexpr size_t MAX_SEARCH_THREADS = 64;

constexpr size_t DEFAULT_MULTI_PV = 1;
constexpr size_t MIN_MULTI_PV = 1;
constexpr size_t MAX_MULTI_PV = 32;

constexpr Depth DEFAULT_MUTE_PLIES = 0;
constexpr Depth MIN_MUTE_PLIES = 0;
constexpr Depth MAX_MUTE_PLIES = 32;
#pragma endregion

#pragma region ===== Evaluation =====
constexpr Score POSITIVE_INF = 32000;
constexpr Score NEGATIVE_INF = -32000;
constexpr Score MATE_SCORE = 31000;
constexpr Score DRAW_SCORE = 0;
constexpr Score MIN_MATE_EVAL = MATE_SCORE - MAX_SEARCH_DEPTH;
constexpr Score KILLER_EVAL_BONUS = 5000;
constexpr Score TT_EVAL_BONUS = 100000;
#pragma endregion

#pragma region ===== Transposition table =====
constexpr uint64_t ZOBRIST_SEED = 7299078832781365792;
constexpr size_t TT_ENTRIES_PER_BUCKET = 4;
constexpr size_t DEFAULT_HASH_SIZE = 128;
constexpr size_t MIN_HASH_SIZE = 8; // this should never be 0
constexpr size_t MAX_HASH_SIZE = 32768;
#pragma endregion

#pragma region ===== Search =====
constexpr TimeRep CURRMOVE_DELAY = 1000;
constexpr TimeRep DEFAULT_SEARCH_TIME = 60000;
constexpr Score FUTILITY_FACTOR = 90;
constexpr Depth FUTILITY_MAX_DEPTH = 6;
constexpr Depth NULL_MIN_DEPTH = 4;
constexpr Depth NULL_BASE_REDUCTION = 4;
constexpr Depth LMR_MIN_DEPTH = 3;
constexpr size_t LMR_MIN_MOVES_SEARCHED = 2;
constexpr size_t KILLER_SLOTS = 2;
#pragma endregion

#endif //MEETRA_CONFIG_H
