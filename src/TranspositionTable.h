#ifndef MEETRA_TRANSPOSITIONTABLE_H
#define MEETRA_TRANSPOSITIONTABLE_H

#include "Types.h"
#include "Bitboards.h"
#include "Board.h"

namespace Meetra {

    enum TTSize : size_t {
        TT256MB = 16000000,
        TT128MB = 8000000,
        TT64MB = 4000000,
        TT32MB = 2000000,
        TT16MB = 1000000,
        TT8MB = 500000,
    };

    enum EntryFlag : Score {
        EXACT_SCORE, ALPHA, BETA, NOT_FOUND = 31123
    };

    class TranspositionTable {

    public:

        explicit TranspositionTable(size_t size);
        void AddEntry(ZobristHash key, Score score, Depth depth, Move move, uint8_t flag);
        Score GetEval(ZobristHash key, Score alpha, Score beta, Depth depth);
        void Resize(size_t new_size);
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

            void SaveEntry(ZobristHash k, Score s, Depth d, Move m, uint8_t f) {
                key = static_cast<uint64_t>(k);
                score = static_cast<int16_t>(s);
                depth = static_cast<uint16_t>(d);
                move = static_cast<uint16_t>(m);
                flag = static_cast<uint8_t>(f);
            }
        };

        size_t size;
        size_t count;
        TTEntry *table;
    };

}

#endif //MEETRA_TRANSPOSITIONTABLE_H
