#include "TranspositionTable.h"
#include "Search.h"

namespace Meetra {

    Score RemoveMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score + ply : score < -MIN_MATE_EVAL ? score - ply : score;
    }

    Score AddMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score - ply : score < -MIN_MATE_EVAL ? score + ply : score;
    }

    void TranspositionTable::Init(size_t size_mb) {

        if (size_mb > MAX_HASH_SIZE || size_mb < MIN_HASH_SIZE) {
            size_mb = std::clamp(size_mb, size_t(MIN_HASH_SIZE), size_t(MAX_HASH_SIZE));
            Uci::SendInfo("Invalid TT size! Initializing to: " + std::to_string(size_mb) + "MB");
        }

        buckets_count = (size_mb * 1048576) / sizeof(TTBucket);
        table = std::make_unique<TTBucket[]>(buckets_count);

        Clear();
    }

    void TranspositionTable::NewSearch() {
        current_epoch++;
        used_entries = 0;
        if (current_epoch >= 64) {
            Clear();
        }
    }

    void TranspositionTable::Clear() {
        used_entries = 0;
        current_epoch = 0;
        std::fill_n(table.get(), buckets_count, TTBucket());
    }

    void TranspositionTable::Save(Hash64 key, Score score, Depth depth, Move move, TTFlag flag, Depth ply) {

        if (buckets_count == 0) {
            return;
        }

        TTBucket &bucket = table[key % buckets_count];
        TTEntry *entry_to_write;
        Hash32 key_32 = Zobrist::MakeHash32(key);
        int worst_entry_score = INT_MAX;

        for (auto &entry: bucket.bucket_entries) {

            if (entry.Get32Key() == key_32) {
                if (entry.GetEpoch() != current_epoch || entry.GetDepth() <= depth) {
                    entry_to_write = &entry;
                } else {
                    entry_to_write = nullptr;
                }
                break;
            }

            int entry_score = static_cast<int>(entry.GetDepth());
            entry_score -= (static_cast<int>(current_epoch) - static_cast<int>(entry.GetEpoch())) * 2;
            if (entry.GetFlag() == EXACT_SCORE) {
                entry_score += 2;
            }

            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = &entry;
            }
        }

        if (entry_to_write) {
            if (entry_to_write->GetEpoch() != current_epoch) {
                used_entries.fetch_add(1, std::memory_order::relaxed);
            }
            entry_to_write->SaveEntry(key_32, RemoveMatePly(score, ply), depth, move, flag, current_epoch);
        }
    }

    void TranspositionTable::Probe(Hash64 key, Score alpha, Score beta,
                                   Depth depth, Depth ply, Score &score, TTFlag &flag, Move &move) const {

        flag = NOT_FOUND;
        move = ZERO_MOVE;

        if (buckets_count == 0) {
            return;
        }

        Hash32 key_32 = Zobrist::MakeHash32(key);
        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry: bucket.bucket_entries) {
            if (entry.Get32Key() == key_32) {
                move = entry.GetMove();
                if (entry.GetDepth() >= depth) {
                    score = AddMatePly(entry.GetScore(), ply);
                    if (entry.GetFlag() == ALPHA && score <= alpha) {
                        entry.SetEpoch(current_epoch);
                        score = alpha;
                        flag = ALPHA;
                    } else if (entry.GetFlag() == BETA && score >= beta) {
                        entry.SetEpoch(current_epoch);
                        score = beta;
                        flag = BETA;
                    } else if (entry.GetFlag() == EXACT_SCORE) {
                        entry.SetEpoch(current_epoch);
                        flag = EXACT_SCORE;
                    }
                }
                return;
            }
        }
    }
}
