#include "MoveList.h"


namespace Meetra {

    // Maybe initialize whole array to 0s so we dont have to make the if quiet check and just return
    // the 0, which is Invalid move
    // then we would have 127 spaces for captures + 1 that would have to remain to be 0, dividing the two halves
    // acting as an invalid move when we run out of quiet moves
    // and 128 quiet moves

    // oooor we just rely on move generator, to generate captures first, and then quiet, so we can have them
    // immediatelly after each other, having to do 0 checks (only adding invalid move all the way at the end)
    inline constexpr Meetra::Move Meetra::MoveList::GetNextMove() {
        if(move_count > 0) return moves[--move_count];
        return INVALID_MOVE;

        /*        if(captures_count > 0){
            return moves[captures_count--];
        }
        if(quiets_count > 128){
            return moves[quiets_count--];
        }*/
        //return INVALID_MOVE;
    }

    constexpr void MoveList::AddMove(Move m){
        moves[move_count++] = m;
    }

    constexpr void MoveList::AddCapture(Move m) {
        //moves[captures_count++] = m;
    }
    constexpr void MoveList::AddQuiet(Move m) {
        //moves[quiets_count++] = m;
    }

}
