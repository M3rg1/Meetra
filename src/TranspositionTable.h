#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include <memory>
#include <atomic>
#include "Board.h"
#include "ZobristHash.h"
#include "Defs.h"
#include "Config.h"

class TranspositionTable {

public:

    enum EntryFlag {
        NOT_FOUND, UPPER, LOWER, EXACT, CUTOFF
    };

    void Init(int size_mb = DEFAULT_HASH_SIZE);
    void Save(Hash64 hash, Score score, Depth depth, Move move, EntryFlag flag, Depth ply);
    void NewSearch();
    void Clear();
    EntryFlag Probe(Hash64 hash, Score alpha, Score beta, Depth depth, Depth ply, Score &score, Move &move);
    [[nodiscard]] inline double Usage() const {
        double usage = static_cast<double>(used_entries.load(std::memory_order_relaxed))
                       / static_cast<double>(buckets_count * TT_ENTRIES_PER_BUCKET);
        return std::min(usage, 1.0);
    }

private:

    static constexpr int EPOCH_MASK = 63;
    using TTEpoch = int;

    // 8 bytes
    struct TTEntry {

        [[nodiscard]] inline Hash16 GetHash16() const { return static_cast<Hash16>(hash); }
        [[nodiscard]] inline Score GetScore() const { return static_cast<Score>(score); }
        [[nodiscard]] inline Depth GetDepth() const { return static_cast<Depth>(depth); }
        [[nodiscard]] inline Move GetMove() const { return static_cast<Move>(move); }
        [[nodiscard]] inline EntryFlag GetFlag() const { return static_cast<EntryFlag>(flag); }
        [[nodiscard]] inline TTEpoch GetEpoch() const { return static_cast<TTEpoch>(epoch); }
        inline void SetEpoch(TTEpoch e) { epoch = e; }

        inline void SaveEntry(Hash16 h, Score s, Depth d, Move m, EntryFlag f, TTEpoch e) {
            hash = static_cast<uint16_t>(h);
            score = static_cast<int16_t>(s);
            depth = static_cast<uint8_t>(d);
            move = static_cast<uint16_t>(m);
            flag = static_cast<uint8_t>(f);
            epoch = static_cast<uint8_t>(e);
        }

    private:
        uint16_t hash = 0;
        int16_t score = 0;
        uint16_t move = 0;
        uint8_t depth = 0;
        uint8_t flag: 2 = 0;
        uint8_t epoch: 6 = 0;  // epoch would be fine with 3 bits, leaving another 3 for whatever else is needed
    };

    struct TTBucket {
        TTEntry entries[TT_ENTRIES_PER_BUCKET];
    };

    TTEpoch current_epoch;
    std::atomic<int_fast64_t> used_entries;
    size_t buckets_count;
    std::unique_ptr<TTBucket[]> table;
};

#endif //MEETRA_TRANSPOSITIONTABLE_H
