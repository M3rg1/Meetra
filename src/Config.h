#ifndef MEETRA_CONFIG_H
#define MEETRA_CONFIG_H

#include "Defs.h"

#include <filesystem>

inline constexpr Depth BOOK_DEPTH = 20; // TODO this should be read from the book itself and put in var in Book.h
inline const std::filesystem::path BOOK_PATH = "books/bestmove_r20d20_a5000.mtr.bin"; // TODO this should be UCI setting (var in Book.h)
inline const std::filesystem::path TEST_FILE_PATH = "tests/PerftTests.txt";

#pragma region ===== Global limits =====
inline constexpr int MAX_LEGAL_MOVES = 256;
inline constexpr int MAX_GAME_LENGTH = 1024;
inline constexpr Depth MAX_SEARCH_DEPTH = 128;
inline constexpr std::string_view STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
#pragma endregion

#pragma region ===== Various UCI settings limits =====
inline constexpr bool DEFAULT_CHESS960 = false;
inline constexpr bool DEFAULT_USE_BOOK = false;
inline constexpr bool DEFAULT_SHOW_CURRLINE = false;
inline constexpr bool DEFAULT_SHOW_CURRMOVE = true;

inline constexpr TimeRep DEFAULT_OVERHEAD = 10;
inline constexpr TimeRep MIN_OVERHEAD = 0;
inline constexpr TimeRep MAX_OVERHEAD = 100000;

inline constexpr TimeRep DEFAULT_UPDATE_INTERVAL = 1000;
inline constexpr TimeRep MIN_UPDATE_INTERVAL = 10;
inline constexpr TimeRep MAX_UPDATE_INTERVAL = 1000000000;

inline constexpr int DEFAULT_SEARCH_THREADS = 1;
inline constexpr int MIN_SEARCH_THREADS = 1;
inline constexpr int MAX_SEARCH_THREADS = 64;

inline constexpr int DEFAULT_MULTI_PV = 1;
inline constexpr int MIN_MULTI_PV = 1;
inline constexpr int MAX_MULTI_PV = 32;

inline constexpr Depth DEFAULT_MUTE_PLIES = 0;
inline constexpr Depth MIN_MUTE_PLIES = 0;
inline constexpr Depth MAX_MUTE_PLIES = MAX_SEARCH_DEPTH;
#pragma endregion

#pragma region ===== Evaluation =====
inline constexpr Score POSITIVE_INF = 32000;
inline constexpr Score NEGATIVE_INF = -32000;
inline constexpr Score MATE_SCORE = 31000;
inline constexpr Score DRAW_SCORE = 0;
inline constexpr Score MIN_MATE_EVAL = MATE_SCORE - MAX_SEARCH_DEPTH;
inline constexpr Score KILLER_EVAL_BONUS = 5000;
inline constexpr Score TT_EVAL_BONUS = 100000;
#pragma endregion

#pragma region ===== Transposition table =====
inline constexpr uint64_t ZOBRIST_SEED = 7299078832781365792;
inline constexpr int TT_ENTRIES_PER_BUCKET = 4;
inline constexpr int DEFAULT_HASH_SIZE = 128;
inline constexpr int MIN_HASH_SIZE = 8; // this should never be 0
inline constexpr int MAX_HASH_SIZE = 32768;
#pragma endregion

#pragma region ===== Search =====
inline constexpr TimeRep CURRMOVE_DELAY = 1000;
inline constexpr TimeRep DEFAULT_SEARCH_TIME = 60000;
inline constexpr Score FUTILITY_FACTOR = 90;
inline constexpr Depth FUTILITY_MAX_DEPTH = 6;
inline constexpr Depth NULL_MIN_DEPTH = 4;
inline constexpr Depth NULL_BASE_REDUCTION = 4;
inline constexpr Depth LMR_MIN_DEPTH = 3;
inline constexpr int LMR_MIN_MOVES_SEARCHED = 2;
inline constexpr int KILLER_SLOTS = 2; // this should never be 0
inline constexpr int TIME_QUERY_FREQUENCY = 16384;
#pragma endregion

#endif //MEETRA_CONFIG_H
