#include <cstring>
#include "TranspositionTable.h"
#include "Evaluation.h"
//#include "omp.h"

namespace Meetra {

#define BUCKET_SIZE 4


    TranspositionTable::TranspositionTable(size_t size) : size(size) {
        table = new TTEntry[sizeof(TTEntry) * size];
        index_mask = size - 1;
        Clear();
    }

    void TranspositionTable::SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag, Depth ply) {

        Key32 key_32 = Make32Key(key);
        TTEntry *entry_to_write;
        int worst_entry_score = 1000000;
        score = RemoveMatePly(score, ply);

//#pragma omp critical
        //{
            for (auto i = 0; i < BUCKET_SIZE; i++) {
                auto index = (key + i) & index_mask;
                // TODO a good way to test if my & index is working is to let it analyze for a long time
                //  and see if it reaches 100% usage
                //  actually better to just print the entries var, since usage is rounded
                TTEntry *curr_entry = &table[index];

                if (curr_entry->Get32Key() == key_32) {
                    if (curr_entry->GetEpoch() != current_epoch || flag == EXACT_SCORE ||
                        (depth > curr_entry->GetDepth() && curr_entry->GetFlag() != EXACT_SCORE)) {
                        entry_to_write = curr_entry;
                    } else {
                        entry_to_write = nullptr;
                    }
                    break;
                }

                int entry_score = static_cast<int>(curr_entry->GetDepth());
                if (curr_entry->GetEpoch() < current_epoch) {
                    if (curr_entry->GetEpoch() == 0) {
                        entry_to_write = curr_entry;
                        break;
                    } else {
                        entry_score -= (current_epoch - curr_entry->GetEpoch()) << 6;
                    }
                }
                // two epochs back = 2 << 6 = 128, but 3 = 192 == keeping exact entries 2 epochs old
                if (curr_entry->GetFlag() == EXACT_SCORE) {
                    entry_score += 150;
                }
                if (entry_score < worst_entry_score) {
                    worst_entry_score = entry_score;
                    entry_to_write = curr_entry;
                }
            }

            if (entry_to_write) {
                if (entry_to_write->GetEpoch() != current_epoch) {
                    entries++;
                }
                entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);
            }
        }
    //}

    Score TranspositionTable::RemoveMatePly(Score score, Depth ply) const {
        if (score > MATE_SCORE - MAX_SEARCH_DEPTH) {
            return score + ply;
        } else if (score < -MATE_SCORE + MAX_SEARCH_DEPTH) {
            return score - ply;
        } else {
            return score;
        }
    }

    Score TranspositionTable::AddMatePly(Score score, Depth ply) const {
        if (score > MATE_SCORE - MAX_SEARCH_DEPTH) {
            return score - ply;
        } else if (score < -MATE_SCORE + MAX_SEARCH_DEPTH) {
            return score + ply;
        } else {
            return score;
        }
    }

    Score TranspositionTable::ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply) const {

        Key32 key_32 = Make32Key(key);
        auto ret = NOT_FOUND;
        // https://en.cppreference.com/w/cpp/atomic/atomic_flag spinlock imple example
//#pragma omp critical
        //{
            for (auto i = 0; i < BUCKET_SIZE; i++) {
                auto index = (key + i) & index_mask;
                TTEntry *ttEntry = &table[index];

                if (ttEntry->Get32Key() == key_32) {
                    if (ttEntry->GetDepth() >= depth) {
                        ttEntry->SetEpoch(current_epoch);
                        if (ttEntry->GetFlag() == EXACT_SCORE) {
                            ret = AddMatePly(ttEntry->GetScore(), ply);
                        } else if (ttEntry->GetFlag() == ALPHA && ttEntry->GetScore() <= alpha) {
                            ret = alpha;
                        } else if (ttEntry->GetFlag() == BETA && ttEntry->GetScore() >= beta) {
                            ret = beta;
                        }
                    }
                    break;
                }
            }
        //}
        return ret;
    }

    void TranspositionTable::NewSearch() {
        if (current_epoch > 62) {
            Clear();
        }
        entries = 0;
        current_epoch++;
    }

    void TranspositionTable::Resize(TTSize new_size) {
        delete[] table;
        size = new_size;
        index_mask = new_size - 1;
        table = new TTEntry[sizeof(TTEntry) * new_size];
        Clear();
    }

    void TranspositionTable::Clear() {
        entries = 0;
        current_epoch = 0;
        memset(table, 0, sizeof(TTEntry) * size);
    }

    Move TranspositionTable::GetPVMove(ZobristHash key) const {

        Key32 key_32 = Make32Key(key);

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

    TranspositionTable::~TranspositionTable() {
        delete[] table;
    }
}