#include <cstring>
#include "TranspositionTable.h"

namespace Meetra {

#define BUCKET_SIZE 5


    TranspositionTable::TranspositionTable(size_t size) : size(size) {
        table = new TTEntry[sizeof(TTEntry) * size];
        entries = 0;
        current_epoch = 0;
        Clear();
    }

    void TranspositionTable::NewSearch() {
        if (current_epoch == 63) {
            Clear();
        }
        entries = 0;
        current_epoch++;
    }

    Key32 TranspositionTable::Make32Key(ZobristHash zobrist_hash) const {
        return zobrist_hash >> 32;
    }

    void TranspositionTable::SaveEval(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag) {

        Key32 key_32 = Make32Key(key);
        TTEntry *entry_to_write;
        int worst_entry_score = 100000;

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) % size;
            TTEntry *curr_entry = &table[index];

            if (curr_entry->Get32Key() == key_32) {
                // if this node is exact and im trying to put in non-exact node - that shouldnt be allowed?
                // or what if this node is exact but has lower depth?
                // do i just keep all exact nodes from this particular epoch no matter what? sounds werid, what
                // if i improve the exact score? only then save it?
                if (depth >= curr_entry->GetDepth()) {
                    curr_entry->SaveEntry(key_32, score, depth, move, flag, current_epoch);
                } else {
                    curr_entry->SetEpoch(current_epoch);
                }
                return;
            }

            int entry_score = static_cast<int>(curr_entry->GetDepth());
            if (curr_entry->GetEpoch() < current_epoch) {
                entry_score -= 100 * (current_epoch - curr_entry->GetEpoch());
            }
            if (entry_score < worst_entry_score) {
                worst_entry_score = entry_score;
                entry_to_write = curr_entry;
            }
        }

        if (entry_to_write) {
            entry_to_write->SaveEntry(key_32, score, depth, move, flag, current_epoch);
        }


        // TODO i think usage counting could be done with generation - we only count entries++ if they are not
        //  overwriting current generation, if they are overwriting anything else (empty, or old entries) then its
        //  entries++ - that way we cant get above 100% since once is everything from current gen, entries == size
        //entries++;
    }

    Score TranspositionTable::ProbeEval(ZobristHash key, Score alpha, Score beta, Depth depth) const {

        Key32 key_32 = Make32Key(key);

        for (auto i = 0; i < BUCKET_SIZE; i++) {
            auto index = (key + i) % size;
            TTEntry *ttEntry = &table[index];

            if (ttEntry->Get32Key() == key_32) {
                if (ttEntry->GetDepth() >= depth) {
                    ttEntry->SetEpoch(current_epoch);
                    if (ttEntry->GetFlag() == EXACT_SCORE) {
                        return ttEntry->GetScore();
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

    void TranspositionTable::Resize(TTSize new_size) {
        delete[] table;
        size = new_size;
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
            auto index = (key + i) % size;
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