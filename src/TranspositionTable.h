#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Types.h"
#include "Bitboards.h"
#include "Board.h"
#include "ZobristHash.h"
#include <memory>
#include <atomic>

namespace Meetra {

#define TT_NOT_FOUND (-32013)
#define MIN_HASH_SIZE 16
#define DEFAULT_HASH_SIZE 128
#define MAX_HASH_SIZE 4096
#define TT_ENTRIES_PER_BUCKET 4

    enum EntryFlag : uint8_t {
        EXACT_SCORE, ALPHA, BETA
    };

    class TranspositionTable {

    public:

        void Init(size_t size_mb = DEFAULT_HASH_SIZE);
        void SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply);
        void Resize(size_t size_mb);
        void NewSearch();
        void Clear();

        [[nodiscard]] Score ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply) const;
        [[nodiscard]] Move GetPVMove(ZobristHash key) const;
        [[nodiscard]] Move GetAnyMove(ZobristHash key) const;
        // 0.01 == 1% usage, 0.1 == 10% usage, 1 == 100% usage
        [[nodiscard]] inline double Usage() const {
            return static_cast<double>(used_entries) / static_cast<double>(buckets_count * TT_ENTRIES_PER_BUCKET);
        }

    private:
#pragma pack(push, 1)
        // 10 bytes
        class TTEntry {
            uint32_t key;
            int16_t score;
            uint8_t depth;
            uint16_t move;
            uint8_t epoch_and_flag; // 2 low bits for flag, rest for epoch
            // epoch would be fine with 3 bits, leaving another 3 for whatever else is needed
        public:
            [[nodiscard]] inline Key32 Get32Key() const { return static_cast<Key32>(key); }
            [[nodiscard]] inline Score GetScore() const { return static_cast<Score>(score); }
            [[nodiscard]] inline Depth GetDepth() const { return static_cast<Depth>(depth); }
            [[nodiscard]] inline Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] inline EntryFlag GetFlag() const { return static_cast<EntryFlag>(epoch_and_flag & 0x3); }
            [[nodiscard]] inline Epoch GetEpoch() const { return static_cast<Epoch>(epoch_and_flag >> 2); }
            [[nodiscard]] inline bool IsEmpty() const { return GetEpoch() == 0; }

            inline void SetEpoch(Epoch e) {
                epoch_and_flag &= 0x3;
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }

            inline void SaveEntry(Key32 k, Score s, Depth d, Move m, EntryFlag f, Epoch e) {
                key = static_cast<uint32_t>(k);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint8_t>(d);
                move = static_cast<uint16_t>(m);
                epoch_and_flag = static_cast<uint8_t>(f);
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }
        };

        class TTBucket {

        public:
            TTEntry bucket_entries[TT_ENTRIES_PER_BUCKET];

            inline void Lock() {
                while (lock.test_and_set(std::memory_order_acquire)) {
                    while (lock.test(std::memory_order_relaxed))
                        ;
                }
            }

            inline void Unlock() { lock.clear(std::memory_order_release); }

        private:
            std::atomic_flag lock;

        };
#pragma pack(pop)

        Epoch current_epoch;
        std::atomic<size_t> used_entries;
        size_t buckets_count;
        std::unique_ptr<TTBucket[]> table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
