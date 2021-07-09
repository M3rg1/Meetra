#include <cstring>
#include "TranspositionTable.h"

namespace Meetra {


    TranspositionTable::TranspositionTable(size_t size) : size(size) {
        table = new TTEntry[sizeof(TTEntry) * size];
        entries = 0;
        Clear();
    }

    void TranspositionTable::NewSearch(){
        entries = 0;
    }

    void TranspositionTable::AddEntry(ZobristHash key, Score score, Depth depth, Move move, EntryFlag flag) {

        TTEntry *ttEntry = &table[key % size];

        if (ttEntry->GetKey() == key && ttEntry->GetDepth() >= depth) {
            return;
        }

        entries++;

        ttEntry->SaveEntry(key, score, depth, move, flag);
    }

    void TranspositionTable::Resize(TTSize new_size) {
        delete[] table;
        size = new_size;
        table = new TTEntry[sizeof(TTEntry) * new_size];
        Clear();
    }

    void TranspositionTable::Clear() {
        entries = 0;
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