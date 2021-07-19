#ifndef MEETRA_PVTABLE_H
#define MEETRA_PVTABLE_H

#include "Types.h"
#include "ZobristHash.h"
#include <memory>
#include <cstring>
#include "iostream"
#include <atomic>

namespace Meetra {

    class PVTable {

#define PVT_BUCKET_SIZE 8
#define PVT_BUCKETS_COUNT 10000

    public:

        PVTable() {
            size = PVT_BUCKETS_COUNT;
            table = std::make_unique<PVBucket[]>(size);
            current_epoch = 0;
            memset(table.get(), 0, sizeof(PVBucket) * size);
        }

        void NewSearch() {
            current_epoch %= 255;
            current_epoch++;
        }

        void AddEntry(ZobristHash k, Move m) {
            PVBucket *bucket = &table[k % size];
            k = Zobrist::Make56Key(k);
            for (auto &e : bucket->bucket_entries) {
                if (e.Get56Key() == k || e.GetEpoch() != current_epoch) {
                    e.SaveEntry(m, k, current_epoch);
                    return;
                }
            }
        }

        [[nodiscard]] Move ProbePv(ZobristHash k) const {
            PVBucket *bucket = &table[k % size];
            k = Zobrist::Make56Key(k);
            for (auto &e : bucket->bucket_entries) {
                if (e.Get56Key() == k) {
                    e.SetEpoch(current_epoch);
                    return e.GetMove();
                }
            }
            return INVALID_MOVE;
        }


    private:

#pragma pack(push, 1)
        class PVEntry {

        private:
            uint16_t move;
            uint64_t key_epoch;

        public:
            [[nodiscard]] inline Move GetMove() const { return static_cast<Move>(move); }
            [[nodiscard]] inline uint64_t Get56Key() const { return key_epoch & 0xFFFFFFFFFFFFFF; }
            [[nodiscard]] inline Epoch GetEpoch() const { return static_cast<Epoch>(key_epoch >> 56); }

            inline void SaveEntry(Move m, ZobristHash k, Epoch e) {
                move = static_cast<uint16_t>(m);
                key_epoch = static_cast<uint64_t>(k & 0xFFFFFFFFFFFFFF);
                key_epoch |= static_cast<uint64_t>(e) << 56;
            }

            inline void SetEpoch(Epoch e) {
                key_epoch &= (static_cast<uint64_t>(e) << 56) | 0xFFFFFFFFFFFFFF;
            }

        };

        struct PVBucket {
            PVEntry bucket_entries[PVT_BUCKET_SIZE];
            std::atomic_flag lock;
        };
#pragma pack(pop)

        int size;
        Epoch current_epoch;
        std::unique_ptr<PVBucket[]> table;

    };

}


#endif //MEETRA_PVTABLE_H
