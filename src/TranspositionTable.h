#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Types.h"
#include "Bitboards.h"
#include "Board.h"
#include "ZobristHash.h"

namespace Meetra {

    enum TTSize : size_t {
        TT512MB =   67108864, //51200000,
        TT256MB =   33554432, //25600000,
        TT128MB =   16777216, //12800000,
        TT64MB =    8388608, //6400000,
        TT32MB =    4194304, //3200000,
        TT16MB =    2097152, //1600000,
        TT8MB =     1048576, //800000
    };

#define DEFAULT_TT_SIZE TT64MB

    enum EntryFlag : uint8_t {
        EXACT_SCORE, ALPHA, BETA
    };
    typedef uint8_t Epoch;

    constexpr Score NOT_FOUND = -32013;

    class TranspositionTable {

    public:

        explicit TranspositionTable(size_t size = DEFAULT_TT_SIZE);
        void SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply);
        void Resize(size_t new_size_mb);
        void NewSearch();
        void Clear();

        [[nodiscard]] Score ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply) const;
        [[nodiscard]] Move GetPVMove(ZobristHash key) const;
        [[nodiscard]] size_t EntriesCount() const { return used_entries; }
        // 0.01 == 1% usage, 0.1 == 10% usage, 1 == 100% usage
        [[nodiscard]] double Usage() const {
            return static_cast<double>(used_entries) / static_cast<double>(size_entries);
        }
        ~TranspositionTable();


    private:

        [[nodiscard]] Score RemoveMatePly(Score score, Depth ply) const;
        [[nodiscard]] Score AddMatePly(Score score, Depth ply) const;

#pragma pack(push, 1)
        // 10 bytes
        class TTEntry {
            uint32_t key;
            int16_t score;
            uint8_t depth;
            uint16_t move;
            uint8_t epoch_and_flag; // 2 low bits for flag, rest for epoch

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

        Epoch current_epoch;
        size_t used_entries;
        size_t size_entries;
        size_t index_mask;
        TTEntry *table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
