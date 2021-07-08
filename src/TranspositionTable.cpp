#include <cstring>
#include "TranspositionTable.h"

namespace Meetra {


    TranspositionTable::TranspositionTable(size_t size) : size(size) {
        count = 0;
        table = new TTEntry[sizeof(TTEntry) * size];
        Clear();
    }

    void TranspositionTable::AddEntry(ZobristHash key, Score score, Depth depth, Move move, uint8_t flag) {
        table[key % size].SaveEntry(key, score, depth, move, flag);
        count++;
    }

    void TranspositionTable::Resize(size_t new_size) {
        delete[] table;
        count = 0;
        size = new_size;
        table = new TTEntry[sizeof(TTEntry) * new_size];
        Clear();
    }

    void TranspositionTable::Clear() {
        memset(table, 0, sizeof(TTEntry) * size);
    }

    Score TranspositionTable::GetEval(ZobristHash key, Score alpha, Score beta, Depth depth) {
        TTEntry ttEntry = table[key % size];
        if (ttEntry.GetKey() == key && ttEntry.GetDepth() >= depth) {
            if (ttEntry.GetFlag() == EXACT_SCORE) {
                return ttEntry.GetScore();
            } else if (ttEntry.GetFlag() == ALPHA && ttEntry.GetScore() <= alpha) {
                return alpha;
            } else if (ttEntry.GetFlag() == BETA && ttEntry.GetScore() >= beta) {
                return beta;
            }
        }
        return NOT_FOUND;
    }
    TranspositionTable::~TranspositionTable() {
        delete []table;
    }
}