#ifndef MEETRA_MOVEGEN_H
#define MEETRA_MOVEGEN_H

#include "Types.h"
#include "Board.h"

namespace Meetra {


    class MoveGen {
    public:
        explicit MoveGen(const Board &board);

        template<bool QSearch>
        Move GetNextMove();

        [[nodiscard]] inline bool IsKingInCheck() const{ return checkers; }

    private:
        const Board &board;

        GenPhase genPhase;

        Move moves[128];
        int move_evals[128];
        uint8_t moves_cnt;

        Bitboard checkers;
        Bitboard blockers;
        Bitboard legal_moves;
        Bitboard phase_mask;
        Bitboard enemy_pieces;
        Bitboard all_pieces;
        Bitboard empty_squares;
        Square king_square;

        Color my_color;
        Color enemy_color;

        [[nodiscard]] inline bool Empty() const { return !moves_cnt; }
        inline Move PopMove() { return moves[--moves_cnt]; }
        inline Move PopAtIdx(int idx) {
            Move ret = moves[idx];
            move_evals[idx] = move_evals[--moves_cnt];
            moves[idx] = moves[moves_cnt];
            return ret;
        }
        inline void PutMove(Move m) { moves[moves_cnt++] = m; }
        inline void PutPromMoves(Square from, Square to){
            PutMove(NewMove(from, to, PROMOTE_QUEEN));
            PutMove(NewMove(from, to, PROMOTE_ROOK));
            PutMove(NewMove(from, to, PROMOTE_BISHOP));
            PutMove(NewMove(from, to, PROMOTE_KNIGHT));
        }

        template<Color C, bool QSearch>
        void NextPhase();

        template<GenPhase phase, Color C>
        void GenMovesForPhase();

        template<PieceType PT, Color C>
        void GenMovesForPieceType(Bitboard legality_mask);

        template <Color C>
        void GenEnPassantMoves();

        template<Color C>
        void GenPawnCaptures();

        template<Color C>
        void GenPawnForwardMoves();

        template<Color C>
        void GenCastlingMoves();

        template<Color C>
        bool CanCastleLong(CastlingRights cr);

        template<Color C>
        bool CanCastleShort(CastlingRights cr);

        bool DiscoveryCheck(Square origin, Square destination);
    };

    template<Color C>
    constexpr Direction PawnFwdDir() {
        return C == WHITE ? NORTH : SOUTH;
    }

    template<Color C>
    constexpr Direction PawnCaptureLeftDir() {
        return C == WHITE ? NORTH_WEST : SOUTH_EAST;
    }

    template<Color C>
    constexpr Direction PawnCaptureRightDir() {
        return C == WHITE ? NORTH_EAST : SOUTH_WEST;
    }

    template<Color C>
    constexpr Bitboard PromotionRank() {
        return C == WHITE ? 0xFF00000000000000UL : 0xFF000000000000FFUL;
    }

    template<Color C>
    constexpr Bitboard TwoFwdRank() {
        return C == WHITE ? 0x00000000FF000000UL : 0x000000FF00000000UL;
    }

    template<Direction D>
    constexpr Bitboard BitShift(Bitboard b) {
        return D == NORTH ? b << 8 : D == SOUTH ? b >> 8 : D == EAST ? (b & ~0x8080808080808080UL) << 1 :
                                                           D == WEST ? (b & ~0x0101010101010101UL) >> 1 : D ==
                                                                                                          NORTH_EAST ?
                                                                                                          (b &
                                                                                                           ~0x8080808080808080UL)
                                                                                                                  << 9 :
                                                                                                          D ==
                                                                                                          NORTH_WEST ?
                                                                                                          (b &
                                                                                                           ~0x0101010101010101UL)
                                                                                                                  << 7 :
                                                                                                          D ==
                                                                                                          SOUTH_EAST ?
                                                                                                          (b &
                                                                                                           ~0x8080808080808080UL)
                                                                                                                  >> 7 :
                                                                                                          D ==
                                                                                                          SOUTH_WEST ?
                                                                                                          (b &
                                                                                                           ~0x0101010101010101UL)
                                                                                                                  >> 9
                                                                                                                     : 0;
    }

}

#endif //MEETRA_MOVEGEN_H
