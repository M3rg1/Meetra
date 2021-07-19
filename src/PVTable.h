#ifndef MEETRA_PVTABLE_H
#define MEETRA_PVTABLE_H

#include "Types.h"
#include "ZobristHash.h"
#include <memory>
#include <cstring>
#include <atomic>


namespace Meetra {

    class PVTable {

#define PVT_ENTRIES_PER_BUCKET 8
#define PVT_BUCKETS_COUNT 20000

    public:

        PVTable() {
            size = PVT_BUCKETS_COUNT;
            table = std::make_unique<PVBucket[]>(size);
            current_epoch = 0;
            memset(table.get(), 0, sizeof(PVBucket) * size);
        }

        void NewSearch() {
            current_epoch %= 15;
            current_epoch++;
        }

        void Clear() {
            current_epoch = 0;
            memset(table.get(), 0, sizeof(PVBucket) * size);
        }

        void SavePv(ZobristHash k, Move m) {
            PVBucket *bucket = &table[k % size];
            k = Zobrist::Make44Key(k);
            int worst_score = 10000;
            PVEntry *entry_to_write;

            for (auto &e : bucket->bucket_entries) {
                if (e.Get44Key() == k) {
                    entry_to_write = &e;
                    break;
                }
                int entry_score = 0;
                if (e.GetEpoch() != current_epoch) {
                    if (e.GetEpoch() < current_epoch) {
                        entry_score -= (current_epoch - e.GetEpoch()) << 2;
                    } else {
                        entry_score -= (current_epoch + (15 - e.GetEpoch())) << 2;
                    }
                }
                if (entry_score < worst_score) {
                    worst_score = entry_score;
                    entry_to_write = &e;
                }
            }
            entry_to_write->SaveEntry(m, k, current_epoch);
        }

        [[nodiscard]] Move ProbePv(ZobristHash k) const {
            PVBucket *bucket = &table[k % size];
            k = Zobrist::Make44Key(k);
            for (auto &e : bucket->bucket_entries) {
                if (e.Get44Key() == k) {
                    e.SetEpoch(current_epoch);
                    return e.GetMove();
                }
            }
            return INVALID_MOVE;
        }


    private:

//#pragma pack(push, 1)
        class PVEntry {

        public:
            [[nodiscard]] inline Move GetMove() const { return static_cast<Move>(entry >> 4); }
            [[nodiscard]] inline uint64_t Get44Key() const { return entry >> 20; }
            [[nodiscard]] inline Epoch GetEpoch() const { return static_cast<Epoch>(entry & 0xF); }

            inline void SaveEntry(Move m, uint64_t key44, Epoch e) {
                entry = key44 << 20; // 44 bit
                entry |= static_cast<uint64_t>(m) << 4; // 16 bit
                entry |= static_cast<uint64_t>(e); // 4 bit
            }

            inline void SetEpoch(Epoch e) {
                entry &= 0xFFFFFFFFFFFFFFF0;
                entry |= static_cast<uint64_t>(e);
            }

        private:
            uint64_t entry;
        };

        struct PVBucket {
            PVEntry bucket_entries[PVT_ENTRIES_PER_BUCKET];
            std::atomic_flag lock;
        };
//#pragma pack(pop)

        int size;
        Epoch current_epoch;
        std::unique_ptr<PVBucket[]> table;

    };

}


#endif //MEETRA_PVTABLE_H
