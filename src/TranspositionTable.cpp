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
        size_mb = std::clamp(size_mb, static_cast<size_t>(MIN_HASH_SIZE), static_cast<size_t>(MAX_HASH_SIZE));
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

    void TranspositionTable::Resize(size_t size_mb) {
        table.reset();
        Init(size_mb);
    }

    void TranspositionTable::Clear() {
        used_entries = 0;
        current_epoch = 0;
        std::fill_n(table.get(), buckets_count, TTBucket());
    }

    void TranspositionTable::Save(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply) {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTEntry *entry_to_write;
        int worst_entry_score = 1000000;
        score = RemoveMatePly(score, ply);
        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry : bucket.bucket_entries) {

            if (entry.Get32Key() == key_32) {
                if (entry.GetEpoch() != current_epoch || entry.GetDepth() <= depth) {
                    entry_to_write = &entry;
                    if (entry_to_write->GetEpoch() != current_epoch) {
                        used_entries.fetch_add(1, std::memory_order::relaxed);
                    }
                    entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);
                }
                return;
            }

            int entry_score = static_cast<int>(entry.GetDepth());
            entry_score -= (static_cast<int>(current_epoch) - static_cast<int>(entry.GetEpoch())) << 2;
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
    }

    void TranspositionTable::Probe(ZobristHash key, Score alpha, Score beta,
                                   Depth depth, Depth ply, Score &score, EntryFlag &flag, Move &move) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        flag = NOT_FOUND;
        move = ZERO_MOVE;

        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry : bucket.bucket_entries) {
            if (entry.Get32Key() == key_32) {
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

    Move TranspositionTable::GetPVMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry : bucket.bucket_entries) {
            if (entry.Get32Key() == key_32) {
                if (entry.GetFlag() == EXACT_SCORE) {
                    return entry.GetMove();
                }
                return ZERO_MOVE;
            }
        }
        return ZERO_MOVE;
    }

    Move TranspositionTable::GetAnyMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTBucket &bucket = table[key % buckets_count];

        for (auto &entry : bucket.bucket_entries) {
            if (entry.Get32Key() == key_32) {
                return entry.GetMove();
            }
        }
        return ZERO_MOVE;
    }
}
