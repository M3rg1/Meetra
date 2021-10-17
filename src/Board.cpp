#include "Board.h"
#include "Bitboards.h"
#include <sstream>
#include "MoveGen.h"
#include <algorithm>
#include "Search.h"
#include <regex>

constexpr Bitboard castling_mask[COLOR_NR]{
        0x00000000000000FF,
        0xFF00000000000000
};

Board::Board() {
    NewPosition(STARTPOS_FEN);
}

bool Board::NewPosition(const std::string &fen) {

    Board previous = *this;

    history_cnt = 0;
    state = BoardState();
    std::ranges::fill(board, NO_PIECE);
    std::ranges::fill(color_bbs, EMPTY_BB);
    std::ranges::fill(type_bbs, EMPTY_BB);
    std::ranges::fill(origin_rooks[BLACK], EMPTY_BB);
    std::ranges::fill(origin_rooks[WHITE], EMPTY_BB);

    if (!ParseFen(fen) || !IsBoardValid()) {
        *this = previous;
        return false;
    }

    state.hash = Zobrist::GenHash64(*this);
    state.evaluator.SetBoard(*this);

    return true;
}

bool Board::IsBoardValid() const {

    Bitboard white_king = GetPieces(KING, WHITE);
    if (!Bitboards::ExactlyOne(white_king)) {
        return false;
    }

    Bitboard black_king = GetPieces(KING, BLACK);
    if (!Bitboards::ExactlyOne(black_king)) {
        return false;
    }

    Square enemy_king_square = ColorToMove() == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
    if (IsAttackedByAny(enemy_king_square, ColorToMove(), GetPieces_pt(ALL_TYPES))) {
        return false;
    }

    if ((state.cr & GetPieces_pt(ROOK)) != state.cr) {
        return false;
    }

    if (CrAvailable(WHITE, SHORT) || CrAvailable(WHITE, LONG)) {
        if (!(white_king & castling_mask[WHITE])) {
            return false;
        }
        if (!Search::chess960 && Bitboards::Lsb(white_king) != E1) {
            return false;
        }
    }

    if (CrAvailable(BLACK, SHORT) || CrAvailable(BLACK, LONG)) {
        if (!(black_king & castling_mask[BLACK])) {
            return false;
        }
        if (!Search::chess960 && Bitboards::Lsb(black_king) != E8) {
            return false;
        }
    }

    if (EpSquare()) {
        Square capture_s = ColorToMove() == WHITE ? EpSquare() + SOUTH : EpSquare() + NORTH;
        Bitboard rank_mask = ColorToMove() == WHITE ? Bitboards::RankMask(RANK_5) : Bitboards::RankMask(RANK_4);
        if (GetPieceOnSquare(capture_s) != NewPiece(PAWN, OtherColor(ColorToMove())) ||
            !(rank_mask & SquareToBB(capture_s))) {
            return false;
        }
    }

    return true;
}

Bitboard Board::PinnedToSquare(Square s, Color blockers_color) const {

    Bitboard all_pieces = GetPieces_pt(ALL_TYPES);
    Bitboard queens = GetPieces(QUEEN, blockers_color);
    Bitboard bishops = GetPieces(BISHOP, blockers_color);
    Bitboard rooks = GetPieces(ROOK, blockers_color);

    Bitboard attackers = ((bishops | queens) & Bitboards::GetBishopRays(s)) |
                         ((rooks | queens) & Bitboards::GetRookRays(s));

    Bitboard pinned_pieces = EMPTY_BB;
    while (attackers) {
        Square attacker_s = Bitboards::PopLsb(attackers);
        Bitboard blocker = Bitboards::GetRayToSquares(attacker_s, s) & all_pieces;
        if (Bitboards::ExactlyOne(blocker)) {
            pinned_pieces |= blocker;
        }
    }

    return pinned_pieces;
}

// takes a pseudo legal move and checks whether it is legal
bool Board::IsMoveLegal(Move m) const {

    if (GetPieceTypeOnSq(FromSquare(m)) == KING) {
        if (GetMoveType(m) == CASTLING) {
            Square from = FromSquare(m);
            Square to = ToSquare(m);
            Move r_move = RookCastlingMove(to, ColorToMove());
            // the xor is for chess960, when king doesn't move, but the rook moving could open an attack on the king
            Bitboard occ = GetPieces_pt(ALL_TYPES) ^ SquareToBB(FromSquare(r_move));
            Bitboard king_walk = Bitboards::GetRayToSquares(from, to) | SquareToBB(to);
            return AllSquaresSafe(king_walk, OtherColor(ColorToMove()), occ);
        }
        Bitboard occ = GetPieces_pt(ALL_TYPES) ^ SquareToBB(FromSquare(m));
        return !IsAttackedByAny(ToSquare(m), OtherColor(ColorToMove()), occ);
    }

    if (GetMoveType(m) == EN_PASSANT) {
        Color my_col = ColorToMove();
        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Square capture_s = my_col == WHITE ? to + SOUTH : to + NORTH;
        Square king_s = Bitboards::Lsb(GetPieces(KING, my_col));
        Bitboard occ = GetPieces_pt(ALL_TYPES) ^ SquareToBB(capture_s) ^ (SquareToBB(from) | SquareToBB(to));
        // ep move can't open us to attacks by anything other than slider pieces
        return !IsAttackedBySliders(king_s, OtherColor(my_col), occ);
    }

    return true;
}

bool Board::IsAttackedByAny(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::GetAttacks<ROOK>(s, occ) &
           (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
           Bitboards::GetAttacks<BISHOP>(s, occ) &
           (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
           Bitboards::GetAttacks<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by) ||
           Bitboards::GetAttacks<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by) ||
           Bitboards::GetAttacks<KING>(s) & GetPieces(KING, attacked_by);
}

Bitboard Board::AttackedBy(Square s, Color attacked_by, Bitboard occ) const {
    return (Bitboards::GetAttacks<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by)) |
           (Bitboards::GetAttacks<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by)) |
           (Bitboards::GetAttacks<BISHOP>(s, occ) &
            (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by))) |
           (Bitboards::GetAttacks<ROOK>(s, occ) &
            (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by))) |
           (Bitboards::GetAttacks<KING>(s) & GetPieces(KING, attacked_by));
}

bool Board::IsAttackedBySliders(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::GetAttacks<ROOK>(s, occ) & (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
           Bitboards::GetAttacks<BISHOP>(s, occ) & (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by));
}

void Board::RemovePiece(Square s) {
    Piece p = board[s];
    board[s] = NO_PIECE;
    Bitboard pos = SquareToBB(s);
    color_bbs[ColorOfPiece(p)] ^= pos;
    type_bbs[TypeOfPiece(p)] ^= pos;
    type_bbs[ALL_TYPES] ^= pos;
}

void Board::AddPiece(Square s, Piece p) {
    board[s] = p;
    Bitboard pos = SquareToBB(s);
    color_bbs[ColorOfPiece(p)] |= pos;
    type_bbs[TypeOfPiece(p)] |= pos;
    type_bbs[ALL_TYPES] |= pos;
}

void Board::MovePiece(Square from, Square to) {
    Piece p = board[from];
    board[from] = NO_PIECE;
    board[to] = p;
    Bitboard from_to = SquareToBB(from) ^ SquareToBB(to);
    color_bbs[ColorOfPiece(p)] ^= from_to;
    type_bbs[TypeOfPiece(p)] ^= from_to;
    type_bbs[ALL_TYPES] ^= from_to;
}

void Board::UnmakeNullMove() {
    state = history[--history_cnt];
}

void Board::MakeNullMove() {
    history[history_cnt++] = state;
    IncrementMoveNumber(ColorToMove());
    ChangeColorToMove();
    Zobrist::UpdateColor(state.hash);
    Zobrist::RemoveEp(state.hash, EpSquare());
    ClearEpSquare();
    ClearCapturedPiece();
    ResetPly();
}

// Make a pseudo legal move.
bool Board::MakeMove(Move m) {

    history[history_cnt++] = state;

    state.evaluator.MakeMove(*this, m);

    Color this_col = ColorToMove();
    ChangeColorToMove();
    Color next_col = ColorToMove();
    Zobrist::UpdateColor(state.hash);

    IncrementMoveNumber(this_col);
    IncrementPly();
    ClearCapturedPiece();
    Zobrist::RemoveEp(state.hash, EpSquare());
    ClearEpSquare();

    Square from = FromSquare(m);
    Square to = ToSquare(m);

    MoveType move_type = GetMoveType(m);

    Square capture_square = move_type == EN_PASSANT ? (this_col == WHITE ? to + SOUTH : to + NORTH) : to;
    Piece captured_piece = GetPieceOnSquare(capture_square);

    // in chess960, when castling, we remove the rook from its square before moving, and put it back later
    // we also need to set captured piece to none, in case the king tries to capture itself
    if (Search::chess960 && move_type == CASTLING) {
        captured_piece = NO_PIECE;
        Move rook_move = RookCastlingMove(to, this_col);
        Square rook_from = FromSquare(rook_move);
        RemovePiece(rook_from);
    }

    if (captured_piece) {
        RemovePiece(capture_square);
        Zobrist::RemovePiece(state.hash, captured_piece, capture_square);
        SetCapturedPiece(captured_piece);
        ResetPly();
    }

    Piece moved_piece = GetPieceOnSquare(from);
    MovePiece(from, to);
    Zobrist::MovePiece(state.hash, moved_piece, from, to);

    Bitboard previous_cr = state.cr;
    state.cr &= ~(SquareToBB(from) | SquareToBB(to));
    Zobrist::UpdateCr(state.hash, previous_cr, GetCr());

    if (TypeOfPiece(moved_piece) == PAWN) {

        ResetPly();

        if (move_type == TWO_FORWARD) {
            SetEpSquare(this_col == WHITE ? to + SOUTH : to + NORTH);
            Zobrist::AddEp(state.hash, EpSquare());
        } else if (IsPromotion(m)) {
            Piece promoted_to = NewPiece(PieceTypeFromFlag(move_type), this_col);
            RemovePiece(to);
            Zobrist::RemovePiece(state.hash, moved_piece, to);
            AddPiece(to, promoted_to);
            Zobrist::AddPiece(state.hash, promoted_to, to);
        } else if (move_type == EN_PASSANT) {
            return !IsAttackedBySliders(Bitboards::Lsb(GetPieces(KING, this_col)), next_col,
                                        GetPieces_pt(ALL_TYPES));
        }

        return true;
    }

    if (TypeOfPiece(moved_piece) == KING) {

        state.cr &= castling_mask[next_col];
        Zobrist::UpdateCr(state.hash, previous_cr, GetCr());

        if (move_type == CASTLING) {
            Move r_move = RookCastlingMove(to, this_col);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            // put back the rook that we took out earlier in chess 960 castling
            Search::chess960 ? AddPiece(r_to, NewPiece(ROOK, this_col)) : MovePiece(r_from, r_to);
            Zobrist::MovePiece(state.hash, NewPiece(ROOK, this_col), r_from, r_to);
            Bitboard king_walk = Bitboards::GetRayToSquares(from, to) | SquareToBB(to);
            return AllSquaresSafe(king_walk, next_col, GetPieces_pt(ALL_TYPES));
        }

        return !IsAttackedByAny(to, next_col, GetPieces_pt(ALL_TYPES));
    }

    return true;
}

void Board::UnmakeMove(Move m) {

    Square from = FromSquare(m);
    Square to = ToSquare(m);
    Piece captured_piece = CapturedPiece();

    state = history[--history_cnt];

    // ches960 castling is a special case
    if (Search::chess960 && GetMoveType(m) == CASTLING) {
        Move rook_move = RookCastlingMove(to, ColorToMove());
        RemovePiece(ToSquare(rook_move));
        MovePiece(to, from);
        AddPiece(FromSquare(rook_move), NewPiece(ROOK, ColorToMove()));
        return;
    }

    MovePiece(to, from);

    if (captured_piece) {
        Square capture_s = GetMoveType(m) == EN_PASSANT ? (ColorToMove() == WHITE ? to + SOUTH : to + NORTH) : to;
        AddPiece(capture_s, captured_piece);
    }

    if (IsPromotion(m)) {
        RemovePiece(from);
        AddPiece(from, NewPiece(PAWN, ColorToMove()));
    } else if (GetMoveType(m) == CASTLING) {
        Move rook_move = RookCastlingMove(to, ColorToMove());
        MovePiece(ToSquare(rook_move), FromSquare(rook_move));
    }
}

bool Board::AllSquaresSafe(Bitboard squares, Color attacker, Bitboard occ) const {
    while (squares) {
        Square s = Bitboards::PopLsb(squares);
        if (IsAttackedByAny(s, attacker, occ)) {
            return false;
        }
    }
    return true;
}

bool Board::IsRepetition() const {
    int stop = std::max(static_cast<int>(HistorySize()) - Ply(), 0);
    for (int i = static_cast<int>(HistorySize()) - 2, rep = 0; i >= stop; i -= 2) {
        if (history[i].hash == state.hash) {
            if (++rep > 1) {
                return true;
            }
        }
    }
    return false;
}

Move Board::RookCastlingMove(Square king_to, Color c) const {
    Square to = FileFromSquare(king_to) == FILE_G ? king_to - 1 : king_to + 1;
    Bitboard rook_pos = FileFromSquare(to) == FILE_F ? origin_rooks[c][SHORT] : origin_rooks[c][LONG];
    return NewMove(Bitboards::Lsb(rook_pos), to);
}

bool Board::ParseFen(const std::string &fen) {

    static const std::regex rgx(R"(\s*([rnbqkpRNBQKP1-8]{1,8}\/){7}([rnbqkpRNBQKP1-8]{1,8})\s*[bw]\s*(([a-hkqA-HKQ]{1,4})|(-))?\s*(([a-h][36])|(-))?\s*\d*\s*\d*\s*)");
    if (!std::regex_match(fen, rgx)) {
        return false;
    }

    std::istringstream iss(fen);
    std::string token;

    iss >> token;
    Square s = A8;
    for (char c: token) {
        if (std::isdigit(c)) {
            s += c - '0';
        } else if (c == '/') {
            if (FileFromSquare(s) != FILE_A) {
                return false;
            }
            s -= 16;
        } else {
            AddPiece(s, CharToPiece(c));
            ++s;
        }
    }
    if (s != H1 + 1) {
        return false;
    }

    iss >> token;
    SetColorToMove(token == "w" ? WHITE : BLACK);

    if (iss >> token && token != "-") {
        for (char c: token) {
            Color col = std::isupper(c) ? WHITE : BLACK;
            Bitboard rooks = GetPieces(ROOK, col);
            rooks &= col == WHITE ? Bitboards::RankMask(RANK_1) : Bitboards::RankMask(RANK_8);
            c = std::tolower(c, std::locale());

            if (c == 'k') {
                Bitboard r_bb = SquareToBB(Bitboards::Msb(rooks));
                state.cr |= r_bb;
                origin_rooks[col][SHORT] = r_bb;
            } else if (c == 'q') {
                Bitboard r_bb = SquareToBB(Bitboards::Lsb(rooks));
                state.cr |= r_bb;
                origin_rooks[col][LONG] = r_bb;
            } else if ('a' <= c && c <= 'h') {
                Bitboard r_bb = SquareToBB(SqFromFiRa(FileFromChar(c), col == WHITE ? RANK_1 : RANK_8));
                state.cr |= r_bb;
                origin_rooks[col][r_bb > GetPieces(KING, col) ? SHORT : LONG] = r_bb;
            }
        }
    }

    if (iss >> token && token != "-") {
        SetEpSquare(SqFromFiRa(FileFromChar(token[0]), RankFromChar(token[1])));
    }

    iss >> state.ply;
    iss >> state.moves;

    return true;
}

bool Board::MakeUciMove(std::string_view move_string) {

    Move uci_move = MoveFromName(move_string);
    MoveGen move_gen(*this);
    Move move;

    while ((move = move_gen.GetAnyMove())) {
        if (FromSquare(move) == FromSquare(uci_move) && ToSquare(move) == ToSquare(uci_move)) {
            if ((IsPromotion(uci_move) || GetMoveType(uci_move) == CASTLING) && move != uci_move) {
                continue;
            }
            MakeMove(move);
            return true;
        }
    }
    return false;
}

std::string Board::MoveToName(Move m) const {

    if (m == ZERO_MOVE) {
        return "0000";
    }

    std::string ret = SquareToName(FromSquare(m)) + SquareToName(ToSquare(m));

    // chess 960 castling is denoted by capturing own rook
    if (Search::chess960 && GetMoveType(m) == CASTLING) {
        Rank r = RankFromSquare(FromSquare(m));
        Color c = r == RANK_1 ? WHITE : BLACK;
        Move r_move = RookCastlingMove(ToSquare(m), c);
        ret = SquareToName(FromSquare(m)) + SquareToName(FromSquare(r_move));
    }

    if (IsPromotion(m)) {
        MoveType prom_flag = GetMoveType(m);
        ret += prom_flag == PROMOTE_QUEEN ? 'q' :
               prom_flag == PROMOTE_ROOK ? 'r' :
               prom_flag == PROMOTE_BISHOP ? 'b' :
               'n';
    }

    return ret;
}

Move Board::MoveFromName(std::string_view move_name) const {

    Square s_from = NameToSquare(&move_name[0]);
    Square s_to = NameToSquare(&move_name[2]);
    MoveType flag = NO_FLAG;

    if (move_name.length() == 5) {
        flag = move_name[4] == 'q' ? PROMOTE_QUEEN :
               move_name[4] == 'r' ? PROMOTE_ROOK :
               move_name[4] == 'b' ? PROMOTE_BISHOP :
               PROMOTE_KNIGHT;
    }

    if (Search::chess960) {
        Piece p = GetPieceOnSquare(s_to);
        if (ColorOfPiece(p) == ColorToMove() && TypeOfPiece(p) == ROOK) {
            s_to = s_to > s_from ? ColorToMove() == WHITE ? G1 : G8 : ColorToMove() == WHITE ? C1 : C8;
            flag = CASTLING;
        }
    }

    return NewMove(s_from, s_to, flag);
}

[[maybe_unused]] std::string Board::PPBoard() const {

    std::ostringstream oss;

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        oss << r + 1 << " |";
        for (File f = FILE_A; f <= FILE_H; ++f) {
            oss << ' ' << PieceToChar(GetPieceOnSquare(SqFromFiRa(f, r))) << ' ';
        }
        oss << '\n';
    }
    oss << "---------------------------\n"
        << "  | A  B  C  D  E  F  G  H\n\n"
        << "Player to move: " << (ColorToMove() == WHITE ? "white\n" : "black\n")
        << "Castling rights: ";
    if (!GetCr()) {
        oss << '-';
    } else {
        if (CrAvailable(WHITE, SHORT)) oss << 'K';
        if (CrAvailable(WHITE, LONG)) oss << 'Q';
        if (CrAvailable(BLACK, SHORT)) oss << 'k';
        if (CrAvailable(BLACK, LONG)) oss << 'q';
    }
    oss << " | EP square: " << (EpSquare() ? SquareToName(EpSquare()) : "-") << '\n'
        << "Full-move clock: " << FullMoveClock() << " | Half-move clock: " << Ply();

    return oss.str();
}
