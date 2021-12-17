#include "TranspositionTable.h"
#include "Search.h"
#include <syncstream>
#include <iostream>

Score RemoveMatePly(Score score, Depth ply) {
    return score > MIN_MATE_EVAL ? score + ply : score < -MIN_MATE_EVAL ? score - ply : score;
}

Score AddMatePly(Score score, Depth ply) {
    return score > MIN_MATE_EVAL ? score - ply : score < -MIN_MATE_EVAL ? score + ply : score;
}

void TranspositionTable::Init(int size_mb) {

    if (size_mb > MAX_HASH_SIZE || size_mb < MIN_HASH_SIZE) {
        size_mb = std::clamp(size_mb, MIN_HASH_SIZE, MAX_HASH_SIZE);
        std::osyncstream(std::cout) << "Invalid TT size! Initializing to: " << size_mb << "MB" << std::endl;
    }

    buckets_count = (size_mb * 1048576ull) / sizeof(TTBucket);
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
    std::fill_n(table.get(), buckets_count, TTBucket{});
}

void TranspositionTable::Save(Hash64 hash, Score score, Depth depth, Move move, TTFlag flag, Depth ply) {

    TTBucket &bucket = table[hash % buckets_count];
    TTEntry *entry_to_write;
    Hash16 hash16 = Zobrist::MakeHash16(hash);
    auto worst_entry_score = 1000000;

    for (auto &entry: bucket.entries) {

        if (entry.GetHash16() == hash16) {
            if (entry.GetEpoch() != current_epoch || entry.GetDepth() <= depth || flag == EXACT) {
                entry_to_write = &entry;
                break;
            } else {
                return;
            }
        }

        auto entry_score = entry.GetDepth() - 4 * (current_epoch - entry.GetEpoch());
        if (entry.GetFlag() == EXACT) {
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

TTFlag TranspositionTable::Probe(Hash64 hash, Score alpha, Score beta, Depth depth, Depth ply, Score &score,
                                 Move &move) {

    TTBucket &bucket = table[hash % buckets_count];
    TTFlag flag = NOT_FOUND;
    Hash16 hash16 = Zobrist::MakeHash16(hash);

    for (auto &entry: bucket.entries) {
        if (entry.GetHash16() == hash16) {
            flag = entry.GetFlag();
            move = entry.GetMove();
            score = AddMatePly(entry.GetScore(), ply);
            if (entry.GetDepth() >= depth
                && ((score <= alpha && (flag & UPPER)) || (score >= beta && (flag & LOWER)) || flag == EXACT)) {
                entry.SetEpoch(current_epoch);
                flag = static_cast<TTFlag>(flag | CUTOFF);
            }
            break;
        }
    }

    return flag;
}

