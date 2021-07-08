#include <cstring>
#include "TranspositionTable.h"
#include <iostream>

namespace Meetra {


    TranspositionTable::TranspositionTable(size_t size) : size(size) {
        table = new TTEntry[sizeof(TTEntry) * size];
        new_entries = 0;
        overwrites = 0;
        Clear();
    }

    void TranspositionTable::AddEntry(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag) {

        TTEntry *ttEntry = &table[key % size];

        if (ttEntry->GetKey() == key && ttEntry->GetDepth() >= depth) {
            return;
        }

        if (ttEntry->GetKey() == 0) {
            new_entries++;
        } else {
            overwrites++;
        }

        ttEntry->SaveEntry(key, score, depth, move, flag);
    }

    void TranspositionTable::Resize(size_t new_size) {
        delete[] table;
        size = new_size;
        table = new TTEntry[sizeof(TTEntry) * new_size];
        Clear();
    }

    void TranspositionTable::Clear() {
        overwrites = 0;
        new_entries = 0;
        memset(table, 0, sizeof(TTEntry) * size);
    }

    Score TranspositionTable::GetEval(ZobristHash key, Score alpha, Score beta, Depth depth) const {
        TTEntry *ttEntry = &table[key % size];
        if (ttEntry->GetKey() == key && ttEntry->GetDepth() >= depth) {
            if (ttEntry->GetFlag() == EXACT_SCORE) {
                return ttEntry->GetScore();
            } else if (ttEntry->GetFlag() == ALPHA && ttEntry->GetScore() <= alpha) {
                return alpha;
            } else if (ttEntry->GetFlag() == BETA && ttEntry->GetScore() >= beta) {
                return beta;
            }
        }
        return NOT_FOUND;
    }

    Move TranspositionTable::GetPVMove(ZobristHash key) const {
        TTEntry *ttEntry = &table[key % size];
        return ttEntry->GetKey() == key ? ttEntry->GetMove() : INVALID_MOVE;
    }

    TranspositionTable::~TranspositionTable() {
        delete[] table;
    }
}