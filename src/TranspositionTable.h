#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Types.h"
#include "Bitboards.h"
#include "Board.h"

namespace Meetra {

    /*
     * Therefor to calculate the address or index requires signature modulo number of entries,
     * for power of two sized tables, the lower part of the hash key, masked by an 'and'-instruction accordantly.
     *
     * ////// MAKE IT POWER OF 2 SO THAT MODULO IS EASY TO CALCULATE WITH JUST & MASKING
     *
     * // CAN STORE ONLY UPPER HALF OF THE ZOBRIST KEY FOR COMPARISONS
     * They require detection, realized by storing the signature as part of the hash entry, to check whether a
     * stored entry matches the position while probing. Specially with power of two entry tables, many programmers choose
     * to trade-off space for accuracy and only store that part of the hash key not already considered as index, or even less.
     * https://www.chessprogramming.org/Transposition_Table
     * IT REQUIRES LESS SPACE!!
     */
    enum TTSize : size_t {
        TT256MB = 25600000,
        TT128MB = 12800000,
        TT64MB = 6400000,
        TT32MB = 3200000,
        TT16MB = 1600000,
        TT8MB = 800000,
    };
#define DEFAULT_TT_SIZE TT256MB

    enum EntryFlag : uint8_t {
        EXACT_SCORE, ALPHA, BETA
    };
    typedef uint32_t Key32;
    typedef uint8_t Epoch;

    constexpr Score NOT_FOUND = -32013;

    class TranspositionTable {

    public:

        explicit TranspositionTable(size_t size = DEFAULT_TT_SIZE);
        void AddEntry(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag);
        void Resize(TTSize new_size);
        void NewSearch();
        void Clear();

        [[nodiscard]] Score GetEval(ZobristHash key, Score alpha, Score beta, Depth depth) const;
        [[nodiscard]] Move GetPVMove(ZobristHash key) const;
        [[nodiscard]] int EntriesCount() const { return entries; }
        // 0.01 == 1% usage, 0.1 == 10% usage, 1 == 100% usage
        [[nodiscard]] double Usage() const {
            return static_cast<double>(entries) / static_cast<double>(size);
        }
        ~TranspositionTable();


    private:

        [[nodiscard]] Key32 Make32Key(ZobristHash zobrist_hash) const;

        // 10 bytes (either 8 or 12 or 16 - will get padded by compiler anyway)
#pragma pack(push, 1)
        class TTEntry {
            uint32_t key;
            int16_t score;
            uint8_t depth;
            uint16_t move;
            // 2 low bits for flag, rest for epoch
            uint8_t epoch_and_flag;

        public:
            [[nodiscard]] Key32 Get32Key() const { return static_cast<Key32>(key); }
            [[nodiscard]] Score GetScore() const { return static_cast<Score>(score); }
            [[nodiscard]] Depth GetDepth() const { return static_cast<Depth>(depth); }
            [[nodiscard]] Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] EntryFlag GetFlag() const { return static_cast<EntryFlag>(epoch_and_flag & 0x3); }
            [[nodiscard]] Epoch GetEpoch() const { return static_cast<Epoch>(epoch_and_flag >> 2); }

            void SetEpoch(Epoch e) {
                epoch_and_flag &= 0x3;
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }
            void SaveEntry(Key32 k, Score s, Depth d, Move m, EntryFlag f, Epoch e) {
                key = static_cast<uint32_t>(k);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint8_t>(d);
                move = static_cast<uint16_t>(m);
                epoch_and_flag = static_cast<uint8_t>(f);
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }
        };
#pragma pack(pop)


/*#pragma pack(push, 1)
        class TTEntry {
            uint32_t key;
            int16_t score;
            uint8_t depth;
            uint16_t move;
            uint8_t flag;
            // TODO can be inside the flag - like last 5 bits or whatever, flag only uses 3 bits
            uint8_t epoch;

        public:
            [[nodiscard]] Key32 Get32Key() const { return static_cast<Key32>(key); }
            [[nodiscard]] Score GetScore() const { return static_cast<Score>(score); }
            [[nodiscard]] Depth GetDepth() const { return static_cast<Depth>(depth); }
            [[nodiscard]] Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] EntryFlag GetFlag() const { return static_cast<EntryFlag>(flag); }
            [[nodiscard]] Epoch GetEpoch() const { return static_cast<Epoch>(epoch); }

            void SetEpoch(Epoch e) { epoch = static_cast<uint8_t>(e); }
            void SaveEntry(Key32 k, Score s, Depth d, Move m, EntryFlag f, Epoch e) {
                key = static_cast<uint32_t>(k);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint8_t>(d);
                move = static_cast<uint16_t>(m);
                flag = static_cast<uint8_t>(f);
                epoch = static_cast<uint8_t>(e);
            }
        };
#pragma pack(pop)*/

        Epoch current_epoch;
        size_t entries;
        size_t size;
        TTEntry *table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
