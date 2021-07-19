#include <cstring>
#include "TranspositionTable.h"
#include "Evaluation.h"

namespace Meetra {

#define BUCKET_SIZE 4

    Score RemoveMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score + ply : score < -MIN_MATE_EVAL ? score - ply : score;
    }

    Score AddMatePly(Score score, Depth ply) {
        return score > MIN_MATE_EVAL ? score - ply : score < -MIN_MATE_EVAL ? score + ply : score;
    }

    TranspositionTable::TranspositionTable(size_t size) : size_entries(size) {
        table = std::make_unique<TTEntry[]>(size);
        index_mask = size - 1;
        Clear();
    }

    void TranspositionTable::SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply) {

        Key32 key_32 = Zobrist::Make32Key(key);
        TTEntry *entry_to_write;
        int worst_entry_score = 1000000;
        score = RemoveMatePly(score, ply);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *curr_entry = &table[index];

            if (curr_entry->Get32Key() == key_32) {
                if (curr_entry->GetEpoch() != current_epoch /*|| flag == EXACT_SCORE*/
                    || curr_entry->GetDepth() < depth /*&& curr_entry->GetFlag() != EXACT_SCORE)*/) {
                    /*(flag == EXACT_SCORE && curr_entry->GetFlag() == EXACT_SCORE && depth > curr_entry->GetDepth())
                    || (curr_entry->GetFlag() != EXACT_SCORE && flag == EXACT_SCORE) || (curr_entry->GetFlag() != EXACT_SCORE && depth > curr_entry->GetDepth()))*/
                    entry_to_write = curr_entry;
                    break;
                }
                return;
            }

            int entry_score = static_cast<int>(curr_entry->GetDepth());
            if (curr_entry->GetEpoch() < current_epoch) {
                if (curr_entry->IsEmpty()) {
                    entry_to_write = curr_entry;
                    break;
                }
                entry_score -= (current_epoch - curr_entry->GetEpoch()) << 6;
            }
            // two epochs back = 2 << 6 = 128, but 3 = 192 == keeping exact used_entries 2 epochs old
/*            if (curr_entry->GetFlag() == EXACT_SCORE) {
                entry_score += 150;
            }*/
            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = curr_entry;
            }

        }

        if (entry_to_write->GetEpoch() != current_epoch) {
            used_entries++;
        }
        entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);

    }

    Score
    TranspositionTable::ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply, Move &m) const {

        Key32 key_32 = Zobrist::Make32Key(key);
        m = INVALID_MOVE;

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *ttEntry = &table[index];
            if (ttEntry->Get32Key() == key_32) {
                if (ttEntry->GetDepth() >= depth) {
                    if (ttEntry->GetFlag() == EXACT_SCORE) {
                        ttEntry->SetEpoch(current_epoch);
                        m = ttEntry->GetMove();
                        return AddMatePly(ttEntry->GetScore(), ply);
                    } else if (ttEntry->GetFlag() == ALPHA && ttEntry->GetScore() <= alpha) {
                        ttEntry->SetEpoch(current_epoch);
                        return alpha;
                    } else if (ttEntry->GetFlag() == BETA && ttEntry->GetScore() >= beta) {
                        ttEntry->SetEpoch(current_epoch);
                        return beta;
                    }
/*                    if (ttEntry->GetFlag() == EXACT_SCORE ||
                        (ttEntry->GetFlag() == ALPHA && ttEntry->GetScore() <= alpha) ||
                        (ttEntry->GetFlag() == BETA && ttEntry->GetScore() >= beta)) {
                        ttEntry->SetEpoch(current_epoch);
                        m = ttEntry->GetMove();
                        return AddMatePly(ttEntry->GetScore(), ply);
                    }*/
                }
                return NOT_FOUND;
            }
        }
        return NOT_FOUND;
    }

    void TranspositionTable::NewSearch() {
        if (current_epoch > 62) {
            //Clear();
            current_epoch = 0;
        }
        used_entries = 0;
        current_epoch++;
    }

    void TranspositionTable::Resize(size_t new_size_mb) {
        // TODO convert from size mb to entries count
        table.reset();
        size_entries = new_size_mb;
        index_mask = new_size_mb - 1;
        table = std::make_unique<TTEntry[]>(new_size_mb);
        Clear();
    }

    void TranspositionTable::Clear() {
        used_entries = 0;
        current_epoch = 0;
        memset(table.get(), 0, sizeof(TTEntry) * size_entries);
    }

    Move TranspositionTable::GetPVMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *ttEntry = &table[index];

            if (ttEntry->Get32Key() == key_32) {
                if (ttEntry->GetFlag() == EXACT_SCORE) {
                    return ttEntry->GetMove();
                }
                return INVALID_MOVE;
            }
        }
        return INVALID_MOVE;
    }

    Move TranspositionTable::GetAnyMove(ZobristHash key) const {

        Key32 key_32 = Zobrist::Make32Key(key);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *ttEntry = &table[index];

            if (ttEntry->Get32Key() == key_32) {
                return ttEntry->GetMove();
            }
        }
        return INVALID_MOVE;
    }
}