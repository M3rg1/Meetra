#ifndef MEETRA_PVTABLE_H
#define MEETRA_PVTABLE_H

#include "Types.h"
#include "ZobristHash.h"
#include <memory>
#include <cstring>
#include <atomic>


namespace Meetra {

    class PVTable {

#define PVT_ENTRIES_PER_BUCKET 4
#define PVT_BUCKETS_COUNT 100000

    public:

        PVTable() {
            buckets_count = PVT_BUCKETS_COUNT;
            table = std::make_unique<PVBucket[]>(buckets_count);
            current_epoch = 0;
            memset(table.get(), 0, sizeof(PVBucket) * buckets_count);
        }

        void NewSearch() {
            current_epoch %= 15;
            current_epoch++;
        }

        void Clear() {
            current_epoch = 0;
            memset(table.get(), 0, sizeof(PVBucket) * buckets_count);
        }

        void SavePv(ZobristHash k, Move m) {

            PVBucket *bucket = &table[k % buckets_count];
            k = Zobrist::Make44Key(k);
            int max_epoch_diff = -1;
            PVEntry *entry_to_write;

            bucket->Lock();

            for (auto &e : bucket->bucket_entries) {

                if (e.Get44Key() == k) {
                    entry_to_write = &e;
                    break;
                }

                int epoch_diff = 0;
                if (e.GetEpoch() != current_epoch) {
                    if (current_epoch > e.GetEpoch()) {
                        epoch_diff = (current_epoch - e.GetEpoch());
                    } else {
                        epoch_diff = (current_epoch + (15 - e.GetEpoch()));
                    }
                }
                if (epoch_diff > max_epoch_diff) {
                    max_epoch_diff = epoch_diff;
                    entry_to_write = &e;
                }
            }

            entry_to_write->SaveEntry(m, k, current_epoch);

            bucket->Unlock();
        }

        [[nodiscard]] Move ProbePv(ZobristHash k) const {

            PVBucket *bucket = &table[k % buckets_count];
            k = Zobrist::Make44Key(k);
            Move ret = INVALID_MOVE;

            bucket->Lock();

            for (auto &e : bucket->bucket_entries) {
                if (e.Get44Key() == k) {
                    e.SetEpoch(current_epoch);
                    ret = e.GetMove();
                    break;
                }
            }

            bucket->Unlock();

            return ret;
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

        class PVBucket {

        public:
            PVEntry bucket_entries[PVT_ENTRIES_PER_BUCKET];

            void Lock(){
                while (lock.test_and_set(std::memory_order_acquire)) {
                    while (lock.test(std::memory_order_relaxed));
                }
            }

            void Unlock(){
                lock.clear(std::memory_order_release);
            }

        private:
            std::atomic_flag lock;

        };
//#pragma pack(pop)

        int buckets_count;
        Epoch current_epoch;
        std::unique_ptr<PVBucket[]> table;

    };

}


#endif //MEETRA_PVTABLE_H
