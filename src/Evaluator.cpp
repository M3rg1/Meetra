#include "Evaluator.h"
#include "Bitboards.h"
#include "EvalValues.h"
#include "Board.h"
#include <algorithm>
#include <ranges>

using namespace Evaluation;

cppflow::model Evaluator::model = cppflow::model(std::string("model-1.595"));

void Evaluator::SetBoard(const Board &board) {
    mg.fill(0);
    eg.fill(0);
    phase = 0;

    for (Color c: Colors) {
        for (PieceType pt: PieceTypes) {
            Bitboard pieces = board.GetPieces(pt, c);
            while (pieces) {
                Square s = Bitboards::PopLsb(pieces);
                mg[c] += mg_table[c][pt][s];
                eg[c] += eg_table[c][pt][s];
                phase += phase_inc[pt];
            }
        }
    }

    Color to_move = board.ColorToMove();
    mg_score = mg[to_move] - mg[OtherColor(to_move)];
    eg_score = eg[to_move] - eg[OtherColor(to_move)];
    mg_phase = std::min(phase, 24);
    eg_phase = 24 - mg_phase;
}

void Evaluator::MakeMove(const Board &board, Move m) {

    Color col = board.ColorToMove();
    Color enemy_col = OtherColor(col);
    Square to = ToSquare(m);
    Square from = FromSquare(m);
    Square capture_s = GetMoveType(m) == EN_PASSANT ? (col == WHITE ? to + SOUTH : to + NORTH) : to;
    PieceType moved_pt = board.GetPieceTypeOnSq(from);
    PieceType taken_pt = board.GetPieceTypeOnSq(capture_s);

    mg[col] += mg_table[col][moved_pt][to] - mg_table[col][moved_pt][from];
    eg[col] += eg_table[col][moved_pt][to] - eg_table[col][moved_pt][from];

    if (taken_pt != NONE_PIECE_TYPE) {
        mg[enemy_col] -= mg_table[enemy_col][taken_pt][capture_s];
        eg[enemy_col] -= eg_table[enemy_col][taken_pt][capture_s];
        phase -= phase_inc[taken_pt];
    }

    if (IsPromotion(m)) {
        PieceType prom_to = PromotionTo(GetMoveType(m));
        mg[col] += mg_table[col][prom_to][to] - mg_table[col][PAWN][to];
        eg[col] += eg_table[col][prom_to][to] - eg_table[col][PAWN][to];
        phase += phase_inc[prom_to] - phase_inc[PAWN];
    } else if (GetMoveType(m) == CASTLING) {
        Move r_move = board.RookCastlingMove(to, col);
        Square r_to = ToSquare(r_move);
        Square r_from = FromSquare(r_move);
        mg[col] += mg_table[col][ROOK][r_to] - mg_table[col][ROOK][r_from];
        eg[col] += eg_table[col][ROOK][r_to] - eg_table[col][ROOK][r_from];
    }

    mg_score = mg[enemy_col] - mg[col];
    eg_score = eg[enemy_col] - eg[col];
    mg_phase = std::min(phase, 24);
    eg_phase = 24 - mg_phase;
}

std::vector<uint8_t> Evaluator::GetValues(const Board &board) const {

    // Board board;
    // board.NewPosition(STARTPOS_FEN);
    std::vector<uint8_t> data;
    for (Color c: Colors) {
        for (PieceType pt: PieceTypes) {
            for (Square s: Squares) {
                Piece p = board.GetPieceOnSquare(s);
                if (ColorOfPiece(p) == c && TypeOfPiece(p) == pt) {
                    data.emplace_back(1);
                } else {
                    data.emplace_back(0);
                }
            }
        }
    }
    /*std::cout << data.size() << std::endl;
    int i = 0;
    for (auto e: data) {
        std::cout << int(e) << ' ';
        ++i;
        if ( i % 60 == 0) {
            std::cout << '\n';
        }
        *//*++i;
        if (i % 8 == 0) {
            std::cout << '\n';
        }
        if (i % 64 == 0) {
            std::cout << "\n\n\n";
            i = 0;
        }*//*
    }
    std::cout << std::endl;*/
    return data;
}

Score Evaluator::GetBoardEval(const Board &b) {

    cppflow::tensor tensor(GetValues(b), {768});
    tensor = cppflow::cast(tensor, TF_UINT8, TF_FLOAT);
    tensor = cppflow::expand_dims(tensor, 0);

    auto output = model(tensor);

    auto data = output.get_data<float>();
    // auto cp = 111.714640912 * std::tan(1.5620688421 * data[0]);

    int side = b.ColorToMove() == WHITE ? 1 : -1;
    return static_cast<Score>(data[0] * 100.0) * side;
    //return (mg_score * mg_phase + eg_score * eg_phase) / 24;
}

Score Evaluator::GetMoveEval(const Board &board, Move m) const {

    Color col = board.ColorToMove();
    Square to = ToSquare(m);
    Square from = FromSquare(m);
    Square capture_s = GetMoveType(m) == EN_PASSANT ? (col == WHITE ? to + SOUTH : to + NORTH) : to;
    PieceType moved_pt = board.GetPieceTypeOnSq(from);
    PieceType taken_pt = board.GetPieceTypeOnSq(capture_s);

    Score mg_val = mg_table[col][moved_pt][to] - mg_table[col][moved_pt][from];
    Score eg_val = eg_table[col][moved_pt][to] - eg_table[col][moved_pt][from];

    if (taken_pt != NONE_PIECE_TYPE) {
        mg_val += mg_table[OtherColor(col)][taken_pt][capture_s];
        eg_val += eg_table[OtherColor(col)][taken_pt][capture_s];
    }

    if (IsPromotion(m)) {
        PieceType prom_to = PromotionTo(GetMoveType(m));
        mg_val += mg_table[col][prom_to][to] - mg_table[col][PAWN][to];
        eg_val += eg_table[col][prom_to][to] - eg_table[col][PAWN][to];
    } else if (GetMoveType(m) == CASTLING) {
        Move r_move = board.RookCastlingMove(to, col);
        Square r_to = ToSquare(r_move);
        Square r_from = FromSquare(r_move);
        mg_val += mg_table[col][ROOK][r_to] - mg_table[col][ROOK][r_from];
        eg_val += eg_table[col][ROOK][r_to] - eg_table[col][ROOK][r_from];
    }

    return (mg_val * mg_phase + eg_val * eg_phase) / 24;
}

