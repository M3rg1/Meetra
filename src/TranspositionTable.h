#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Defs.h"
#include "Config.h"

#include <memory>
#include <atomic>

class TranspositionTable {

public:

    enum EntryFlag {
        NOT_FOUND, UPPER, LOWER, EXACT, CUTOFF
    };

    void Init(int size_mb = DEFAULT_HASH_SIZE);
    void NewSearch();
    void Clear();
    void Save(Hash64 hash, Score score, Depth depth, Move move, EntryFlag flag, Depth ply);
    std::tuple<EntryFlag, Score, Move> Probe(Hash64 hash, Score alpha, Score beta, Depth depth, Depth ply);
    [[nodiscard]] int Hashfull() const {
        double usage = static_cast<double>(used_entries.load(std::memory_order_relaxed))
                       / static_cast<double>(buckets_count * TT_ENTRIES_PER_BUCKET);
        return static_cast<int>(std::min(usage * 1000.0, 1000.0));
    }

private:

    static constexpr int EPOCH_MASK = 63;
    using TTEpoch = int;

    // 8 bytes
    struct TTEntry {

        [[nodiscard]] Hash16 GetHash16() const { return static_cast<Hash16>(hash); }
        [[nodiscard]] Score GetScore() const { return static_cast<Score>(score); }
        [[nodiscard]] Depth GetDepth() const { return static_cast<Depth>(depth); }
        [[nodiscard]] Move GetMove() const { return static_cast<Move>(move); }
        [[nodiscard]] EntryFlag GetFlag() const { return static_cast<EntryFlag>(flag); }
        [[nodiscard]] TTEpoch GetEpoch() const { return static_cast<TTEpoch>(epoch); }

        void SetEpoch(TTEpoch e) { epoch = e; }
        void SaveEntry(Hash16 h, Score s, Depth d, Move m, EntryFlag f, TTEpoch e) {
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
    std::atomic<uint64_t> used_entries;
    size_t buckets_count;
    std::unique_ptr<TTBucket[]> table;
};

#endif //MEETRA_TRANSPOSITIONTABLE_H
