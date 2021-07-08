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
        TT256MB = 16000000,
        TT128MB = 8000000,
        TT64MB = 4000000,
        TT32MB = 2000000,
        TT16MB = 1000000,
        TT8MB = 500000,
    };

    enum EntryFlag : Score {
        EXACT_SCORE, ALPHA, BETA, NOT_FOUND = -31123
    };

    class TranspositionTable {

    public:

        explicit TranspositionTable(size_t size);
        void AddEntry(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag);
        void Resize(size_t new_size);
        [[nodiscard]] Score GetEval(ZobristHash key, Score alpha, Score beta, Depth depth) const;
        [[nodiscard]] Move GetPVMove(ZobristHash key) const;
        [[nodiscard]] int Overwrites() const { return overwrites; }
        [[nodiscard]] int NewEntries() const { return new_entries; }
        [[nodiscard]] double Usage() const {
            return static_cast<double>(new_entries) / static_cast<double>(size);
        }
        ~TranspositionTable();


    private:

        void Clear();

        // 16 bytes
        class TTEntry {
            uint64_t key;
            int16_t score;
            uint16_t depth;
            uint16_t move;
            uint8_t flag;

        public:
            [[nodiscard]] ZobristHash GetKey() const { return static_cast<ZobristHash>(key); }
            [[nodiscard]] Score GetScore() const { return static_cast<Score>(score); }
            [[nodiscard]] Depth GetDepth() const { return static_cast<Depth>(depth); }
            [[nodiscard]] Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] EntryFlag GetFlag() const { return static_cast<EntryFlag>(flag); }

            void SaveEntry(ZobristHash k, Score s, Depth d, Move m, EntryFlag f) {
                key = static_cast<uint64_t>(k);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint16_t>(d);
                move = static_cast<uint16_t>(m);
                flag = static_cast<uint8_t>(f);
            }
        };

        int overwrites;
        int new_entries;
        size_t size;
        TTEntry *table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
