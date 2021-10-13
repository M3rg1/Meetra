#include "TranspositionTable.h"
#include "Search.h"
#include "Uci.h"

namespace Meetra {

    Score RemoveMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score + ply : score < -MIN_MATE_EVAL ? score - ply : score;
    }

    Score AddMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score - ply : score < -MIN_MATE_EVAL ? score + ply : score;
    }

    void TranspositionTable::Init(size_t size_mb) {

        if (size_mb > MAX_HASH_SIZE || size_mb < MIN_HASH_SIZE) {
            size_mb = std::clamp(size_mb, MIN_HASH_SIZE, MAX_HASH_SIZE);
            Uci::SendInfo("Invalid TT size! Initializing to: " + std::to_string(size_mb) + "MB");
        }

        buckets_count = (size_mb * 1048576) / sizeof(TTBucket);
        table = std::make_unique<TTBucket[]>(buckets_count);
        Clear();
    }

    void TranspositionTable::NewSearch() {
        if (current_epoch >= 63) {
            Clear();
        }
        used_entries = 0;
        ++current_epoch;
    }

    void TranspositionTable::Clear() {
        used_entries = 0;
        current_epoch = 0;
        std::fill_n(table.get(), buckets_count, TTBucket());
    }

    void TranspositionTable::Save(Hash64 hash, Score score, Depth depth, Move move, TTFlag flag, Depth ply) {

        TTBucket &bucket = table[hash % buckets_count];
        TTEntry *entry_to_write;
        Hash16 hash16 = Zobrist::MakeHash16(hash);
        int worst_entry_score = INT_MAX;

        for (auto &entry: bucket.bucket_entries) {

            if (entry.GetHash16() == hash16) {
                if (entry.GetEpoch() != current_epoch || entry.GetDepth() <= depth || flag == EXACT) {
                    entry_to_write = &entry;
                    break;
                } else {
                    return;
                }
            }

            int entry_score = entry.GetDepth() + 2 * (entry.GetFlag() == EXACT) - 4 * (current_epoch - entry.GetEpoch());
            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = &entry;
            }
        }

        used_entries.fetch_add(entry_to_write->GetEpoch() != current_epoch, std::memory_order::relaxed);
        entry_to_write->SaveEntry(hash16, RemoveMatePly(score, ply), depth, move, flag, current_epoch);
    }

    TTFlag TranspositionTable::Probe(Hash64 hash, Score alpha, Score beta, Depth depth, Depth ply, Score &score,
                                     Move &move) const {

        TTFlag flag = NOT_FOUND;
        Hash16 hash16 = Zobrist::MakeHash16(hash);
        TTBucket &bucket = table[hash % buckets_count];

        for (auto &entry: bucket.bucket_entries) {
            if (entry.GetHash16() == hash16) {
                flag = entry.GetFlag();
                move = entry.GetMove();
                score = AddMatePly(entry.GetScore(), ply);
                if (entry.GetDepth() >= depth &&
                    ((score <= alpha && (flag & ALPHA)) || (score >= beta && (flag & BETA)) || flag == EXACT)) {
                    entry.SetEpoch(current_epoch);
                    flag |= CUTOFF;
                }
                break;
            }
        }
        return flag;
    }
}
