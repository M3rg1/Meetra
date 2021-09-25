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

        TTBucket &bucket = table[key % buckets_count];
        TTEntry *entry_to_write;
        Hash16 hash16 = Zobrist::MakeHash16(key);
        int worst_entry_score = INT_MAX;

        for (auto &entry: bucket.bucket_entries) {

            if (entry.GetHash16() == hash16) {
                if (entry.GetEpoch() != current_epoch || entry.GetDepth() <= depth) {
                    entry_to_write = &entry;
                } else {
                    return;
                }
                break;
            }

            int entry_score = entry.GetDepth();
            entry_score -= (current_epoch - entry.GetEpoch()) * 2;
            if (entry.GetFlag() == EXACT_SCORE) {
                entry_score += 2;
            }

            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = &entry;
            }
        }

        if (entry_to_write->GetEpoch() != current_epoch) {
            used_entries.fetch_add(1, std::memory_order::relaxed);
        }
        entry_to_write->SaveEntry(hash16, RemoveMatePly(score, ply), depth, move, flag, current_epoch);
    }

    void TranspositionTable::Probe(Hash64 key, Score alpha, Score beta,
                                   Depth depth, Depth ply, Score &score, TTFlag &flag, Move &move) const {

        flag = NOT_FOUND;
        Hash16 hash16 = Zobrist::MakeHash16(key);
        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry: bucket.bucket_entries) {
            if (entry.GetHash16() == hash16) {
                flag = MOVE_ONLY;
                move = entry.GetMove();
                score = AddMatePly(entry.GetScore(), ply);
                if (entry.GetDepth() >= depth) {
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
