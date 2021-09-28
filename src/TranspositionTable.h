#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include <memory>
#include <atomic>
#include "Board.h"
#include "ZobristHash.h"

namespace Meetra {

#define MIN_HASH_SIZE 8 // this should never be 0
#define DEFAULT_HASH_SIZE 128
#define MAX_HASH_SIZE 8192
#define TT_ENTRIES_PER_BUCKET 4

    enum TTFlags {
        NOT_FOUND, ALPHA, BETA, EXACT, CUTOFF
    };
    using TTFlag = int;

    class TranspositionTable {

    public:

        void Init(size_t size_mb = DEFAULT_HASH_SIZE);
        void Save(Hash64 hash, Score score, Depth depth, Move move, TTFlag flag, Depth ply);
        void NewSearch();
        void Clear();
        TTFlag Probe(Hash64 hash, Score alpha, Score beta, Depth depth, Depth ply, Score &score, Move &move) const;
        [[nodiscard]] inline double Usage() const {
            if (buckets_count == 0) return 0.0;
            double usage = static_cast<double>(used_entries.load(std::memory_order_relaxed))
                           / static_cast<double>(buckets_count * TT_ENTRIES_PER_BUCKET);
            return std::min(usage, 1.0);
        }

    private:

        using TTEpoch = int;
        // 8 bytes
        class TTEntry {
            uint16_t hash = 0;
            int16_t score = 0;
            uint16_t move = 0;
            uint8_t depth = 0;
            uint8_t epoch_and_flag = 0; // 2 low bits for flag, rest for epoch.
            // epoch would be fine with 3 bits, leaving another 3 for whatever else is needed
        public:
            [[nodiscard]] inline Hash16 GetHash16() const { return static_cast<Hash16>(hash); }
            [[nodiscard]] inline Score GetScore() const { return static_cast<Score>(score); }
            [[nodiscard]] inline Depth GetDepth() const { return static_cast<Depth>(depth); }
            [[nodiscard]] inline Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] inline TTFlag GetFlag() const { return static_cast<TTFlag>(epoch_and_flag & 0x3); }
            [[nodiscard]] inline TTEpoch GetEpoch() const { return static_cast<TTEpoch>(epoch_and_flag >> 2); }

            inline void SetEpoch(TTEpoch e) {
                epoch_and_flag &= 0x3;
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }

            inline void SaveEntry(Hash16 h, Score s, Depth d, Move m, TTFlag f, TTEpoch e) {
                hash = static_cast<uint16_t>(h);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint8_t>(d);
                move = static_cast<uint16_t>(m);
                epoch_and_flag = static_cast<uint8_t>(f);
                epoch_and_flag |= static_cast<uint8_t>(e) << 2;
            }
        };

        struct TTBucket {
            TTEntry bucket_entries[TT_ENTRIES_PER_BUCKET]{};
        };

        TTEpoch current_epoch;
        std::atomic<size_t> used_entries;
        size_t buckets_count;
        std::unique_ptr<TTBucket[]> table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
