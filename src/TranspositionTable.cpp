#include <cstring>
#include "TranspositionTable.h"
#include "Evaluation.h"
#include "Search.h"

namespace Meetra {

    Score RemoveMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score + ply : score < -MIN_MATE_EVAL ? score - ply : score;
    }

    Score AddMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score - ply : score < -MIN_MATE_EVAL ? score + ply : score;
    }

    void TranspositionTable::Init(size_t size_mb) {
        size_mb = std::clamp(size_mb, static_cast<size_t>(MIN_HASH_SIZE), static_cast<size_t>(MAX_HASH_SIZE));
        buckets_count = (size_mb * 1000000) / sizeof(TTBucket);
        table = std::make_unique<TTBucket[]>(buckets_count);
        Clear();
    }

    void TranspositionTable::SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply) {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTEntry *entry_to_write;
        int worst_entry_score = 1000000;
        score = RemoveMatePly(score, ply);
        TTBucket *bucket = &table[key % buckets_count];

        bucket->Lock();

        for (auto &entry : bucket->bucket_entries) {

            if (entry.Get32Key() == key_32) {
                if (entry.GetEpoch() != current_epoch || entry.GetDepth() < depth) {
                    entry_to_write = &entry;
                    if (entry_to_write->GetEpoch() != current_epoch) {
                        used_entries.fetch_add(1, std::memory_order::relaxed);
                    }
                    entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);
                    bucket->Unlock();
                    return;
                }
                bucket->Unlock();
                return;
            }

            int entry_score = static_cast<int>(entry.GetDepth());
            if (entry.GetEpoch() <= current_epoch) {
                entry_score -= (current_epoch - entry.GetEpoch()) << 2;
            } else {
                entry_score -= (current_epoch + (63 - entry.GetEpoch())) << 2;
            }
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
        entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);

        bucket->Unlock();
    }

    Score
    TranspositionTable::ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        Score ret = TT_NOT_FOUND;

        TTBucket *bucket = &table[key % buckets_count];

        bucket->Lock();

        for (auto &entry : bucket->bucket_entries) {
            if (entry.Get32Key() == key_32) {
                if (entry.GetDepth() >= depth) {
                    if (entry.GetFlag() == EXACT_SCORE ||
                        entry.GetFlag() == ALPHA && entry.GetScore() <= alpha ||
                        entry.GetFlag() == BETA && entry.GetScore() >= beta
                            ) {
                        entry.SetEpoch(current_epoch);
                        ret = AddMatePly(entry.GetScore(), ply);
                    }
                }
                break;
            }
        }

        bucket->Unlock();
        return ret;
    }

    void TranspositionTable::NewSearch() {
        current_epoch %= 63;
        current_epoch++;
        used_entries.store(0, std::memory_order::relaxed);
    }

    void TranspositionTable::Resize(size_t size_mb) {
        size_mb = std::clamp(size_mb, static_cast<size_t>(MIN_HASH_SIZE), static_cast<size_t>(MAX_HASH_SIZE));
        table.reset();
        buckets_count = (static_cast<size_t>(size_mb) * 1000000) / sizeof(TTBucket);
        table = std::make_unique<TTBucket[]>(buckets_count);
        Clear();
    }

    void TranspositionTable::Clear() {
        used_entries.store(0, std::memory_order::relaxed);
        current_epoch = 0;
        memset(table.get(), 0, sizeof(TTBucket) * buckets_count);
    }

    Move TranspositionTable::GetPVMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTBucket *bucket = &table[key % buckets_count];
        Move ret = INVALID_MOVE;

        bucket->Lock();

        for (auto &entry : bucket->bucket_entries) {
            if (entry.Get32Key() == key_32) {
                if (entry.GetFlag() == EXACT_SCORE) {
                    ret = entry.GetMove();
                }
                break;
            }
        }

        bucket->Unlock();
        return ret;
    }

    Move TranspositionTable::GetAnyMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTBucket *bucket = &table[key % buckets_count];
        Move ret = INVALID_MOVE;

        bucket->Lock();

        for (auto &entry : bucket->bucket_entries) {
            if (entry.Get32Key() == key_32) {
                ret = entry.GetMove();
            }
        }

        bucket->Unlock();
        return ret;
    }
}