#include "MoveList.h"
#include <iostream>


namespace Meetra {

    // TODO template color + type of move list
    // and then all the mnethods called by this mvoe list can be color templated as well
    MoveList::MoveList(const Board &board, MoveListType t) : board(board) {
        genPhase = BEST_MOVE;
        checkers = SquareAttackers(Lsb(board.GetPieces(KING, board.ColorToMove())),
                                   OtherColor(board.ColorToMove()), board.GetPieces(ALL_TYPES));

        legal_moves = 0xFFFFFFFFFFFFFFFFUL;
        // THIS IS GETTING RECOMPUTED ON EVERY PHASE, ITS ENOUGH TO DO IT ONCE!!!!!!
        if(PopCount(checkers) > 1){
            genPhase = EVASION;
        }
        else if (checkers) {
            Square king_square = Lsb(board.GetPieces(KING, board.ColorToMove()));
            Bitboard capture_mask = checkers;
            Square attacker_square = Lsb(capture_mask);
            Bitboard block_mask = rays_between_squares[king_square][attacker_square];
            legal_moves = capture_mask | block_mask;
        }
    }

    Move MoveList::GetNextMove() {
        while (moves.empty()) {
            if (board.ColorToMove() == WHITE) {
                GenNewMoves<WHITE>();
            } else {
                GenNewMoves<BLACK>();
            }
        }

        // https://www.chessprogramming.org/Move_Ordering -- "Typical move ordering"
        // selection sort to pick the best move - pass through the whole list once and pick move with highest score

        Move m = moves.front();
        moves.pop_front();
        return m;
    }

    inline Bitboard MoveList::SquareAttackers(Square s, Color attacked_by, Bitboard occ) const {
        return (GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & board.GetPieces(PAWN, attacked_by)) |
               (GetAttacksForPiece<KNIGHT>(s) & board.GetPieces(KNIGHT, attacked_by)) |
               (GetAttacksForPiece<BISHOP>(s, occ) & (board.GetPieces(BISHOP, attacked_by) | board.GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<ROOK>(s, occ) & (board.GetPieces(ROOK, attacked_by) | board.GetPieces(QUEEN, attacked_by))) |
               (GetAttacksForPiece<KING>(s) & board.GetPieces(KING, attacked_by));
    }

    // TODO the whole class will be template, for either normal move list or quietsearch move list
    // TODO this function will be templated so that the switch is only

    template<Color C>
    inline void MoveList::GenNewMoves() {
        switch (genPhase) {
            case BEST_MOVE:
                ++genPhase;
                // return TT / killer move // or make case: Killer Move (also from history heuristic possible)
                // also null move? PV? etc.
                break;
            case CAPTURE:
                GenMoves<CAPTURE, C>(board, moves, checkers, legal_moves);
                ++genPhase;
                break;
            case QUIET:
                GenMoves<QUIET, C>(board, moves, checkers, legal_moves);
                ++genPhase;
                break;
            case END:
                moves.emplace_back(INVALID_MOVE);
                break;
            case EVASION:
                GenMoves<EVASION, C>(board, moves, checkers, legal_moves);
                genPhase = END;
                break;
        }
    }

}
