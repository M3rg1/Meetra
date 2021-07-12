#include <cstring>
#include "TranspositionTable.h"
#include "Evaluation.h"

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
        int worst_entry_score = 100000;

        score = RemoveMatePly(score, ply);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *curr_entry = &table[index];

            if (curr_entry->Get32Key() == key_32) {
                if(curr_entry->GetEpoch() != current_epoch){
                    curr_entry->SetEpoch(current_epoch);
                    entries++;
                }
                if (depth >= curr_entry->GetDepth() || flag == EXACT_SCORE) {
                    curr_entry->SaveEntry(key_32, score, depth, move, flag, current_epoch);
                }
                return;
            }

            int entry_score = static_cast<int>(curr_entry->GetDepth());
            if (curr_entry->GetEpoch() < current_epoch) {
                entry_score -= (current_epoch - curr_entry->GetEpoch()) << 5;
            }
            if (curr_entry->GetFlag() == EXACT_SCORE) {
                entry_score += 1000;
            }
            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = curr_entry;
            }
        }

        if(entry_to_write->GetEpoch() != current_epoch){
            entries++;
        }
        entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);
    }

    Score TranspositionTable::RemoveMatePly(Score score, Depth ply) const {
        if(score > MATE_SCORE - MAX_SEARCH_DEPTH){
            return score + ply;
        } else if (score < -MATE_SCORE + MAX_SEARCH_DEPTH) {
            return score - ply;
        } else {
            return score;
        }
    }

    Score TranspositionTable::AddMatePly(Score score, Depth ply) const {
/*        if(std::abs(score) >= MATE_SCORE - MAX_GAME_LENGTH){
            return score + ply;
        }*/

        if(score >= MATE_SCORE - MAX_SEARCH_DEPTH){
            return score - ply;
        } else if (score <= -MATE_SCORE + MAX_SEARCH_DEPTH) {
            return score + ply;
        } else {
            return score;
        }
    }

    Score TranspositionTable::ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth, Depth ply) const {

        Key32 key_32 = Make32Key(key);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) & index_mask;
            TTEntry *ttEntry = &table[index];

            if (ttEntry->Get32Key() == key_32) {
                if (ttEntry->GetDepth() >= depth) {
                    ttEntry->SetEpoch(current_epoch);
                    if (ttEntry->GetFlag() == EXACT_SCORE) {
                        return AddMatePly(ttEntry->GetScore(), ply);
                    } else if (ttEntry->GetFlag() == ALPHA && ttEntry->GetScore() <= alpha) {
                        return alpha;
                    } else if (ttEntry->GetFlag() == BETA && ttEntry->GetScore() >= beta) {
                        return beta;
                    }
                }
                break;
            }
        }

        return NOT_FOUND;
    }

    void TranspositionTable::NewSearch() {
        if (current_epoch == 63) {
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