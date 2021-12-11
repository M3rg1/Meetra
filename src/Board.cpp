#include "Board.h"
#include <sstream>
#include "MoveGen.h"
#include <algorithm>
#include <regex>
#include <iomanip>
#include <bit>
#include <ranges>

Board::Board() {
    NewPosition(STARTPOS_FEN, false);
}

bool Board::NewPosition(const std::string &fen, bool isChess960) {

    Board previous = *this;

    chess960 = isChess960;
    history_cnt = 0;
    uci_moves_cnt = 0;
    state = BoardState();
    std::ranges::fill(board, NO_PIECE);
    std::ranges::fill(color_bbs, EMPTY_BB);
    std::ranges::fill(type_bbs, EMPTY_BB);
    std::ranges::fill(origin_rooks[BLACK], EMPTY_BB);
    std::ranges::fill(origin_rooks[WHITE], EMPTY_BB);

    if (!ParseFen(fen) || !IsValid()) {
        *this = previous;
        return false;
    }

    state.hash = Zobrist::GenHash64(*this);
    state.evaluator.SetBoard(*this);

    Square king_s = Bitboards::Lsb(GetPieces(KING, ColorToMove()));
    state.checkers = AttackedBy(king_s, OtherColor(ColorToMove()), GetPieces(ALL_TYPES));

    return true;
}

bool Board::IsValid() const {

    Bitboard white_king = GetPieces(KING, WHITE);
    Bitboard black_king = GetPieces(KING, BLACK);
    if (!Bitboards::ExactlyOne(white_king) || !Bitboards::ExactlyOne(black_king)) {
        return false;
    }

    Square enemy_king_square = ColorToMove() == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
    if (IsAttackedByAny(enemy_king_square, ColorToMove(), GetPieces(ALL_TYPES))) {
        return false;
    }

    if (CrAvailable(WHITE, SHORT) || CrAvailable(WHITE, LONG)) {
        if (!(white_king & Bitboards::castling_mask[WHITE])) {
            return false;
        }
        if (!chess960 && Bitboards::Lsb(white_king) != E1) {
            return false;
        }
    }

    if (CrAvailable(BLACK, SHORT) || CrAvailable(BLACK, LONG)) {
        if (!(black_king & Bitboards::castling_mask[BLACK])) {
            return false;
        }
        if (!chess960 && Bitboards::Lsb(black_king) != E8) {
            return false;
        }
    }

    if (EpSquare()) {
        Square capture_s = ColorToMove() == WHITE ? EpSquare() + SOUTH : EpSquare() + NORTH;
        Bitboard rank_mask = Bitboards::two_fwd_mask[OtherColor(ColorToMove())];
        if (GetPieceOnSquare(capture_s) != NewPiece(PAWN, OtherColor(ColorToMove()))
            || !(rank_mask & SqToBB(capture_s))) {
            return false;
        }
    }

    if (GetPieces(PAWN) & (Bitboards::RankMask(RANK_8) | Bitboards::RankMask(RANK_1))) {
        return false;
    }

    return true;
}

Bitboard Board::PinnedToSquare(Square s, Color blockers_color) const {

    Bitboard all_pieces = GetPieces(ALL_TYPES);
    Bitboard queens = GetPieces(QUEEN, blockers_color);
    Bitboard bishops = GetPieces(BISHOP, blockers_color);
    Bitboard rooks = GetPieces(ROOK, blockers_color);

    Bitboard attackers = ((bishops | queens) & Bitboards::GetBishopRays(s))
                         | ((rooks | queens) & Bitboards::GetRookRays(s));

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
            Bitboard occ = GetPieces(ALL_TYPES) ^ SqToBB(FromSquare(r_move));
            Bitboard king_walk = Bitboards::GetRayToSquares(from, to) | SqToBB(to);
            return AllSquaresSafe(king_walk, OtherColor(ColorToMove()), occ);
        }
        Bitboard occ = GetPieces(ALL_TYPES) ^ SqToBB(FromSquare(m));
        return !IsAttackedByAny(ToSquare(m), OtherColor(ColorToMove()), occ);
    }

    if (GetMoveType(m) == EN_PASSANT) {
        Color my_col = ColorToMove();
        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Square capture_s = my_col == WHITE ? to + SOUTH : to + NORTH;
        Square king_s = Bitboards::Lsb(GetPieces(KING, my_col));
        Bitboard occ = GetPieces(ALL_TYPES) ^ SqToBB(capture_s) ^ (SqToBB(from) | SqToBB(to));
        // ep move can't open us to attacks by anything other than slider pieces
        return !IsAttackedBySliders(king_s, OtherColor(my_col), occ);
    }

    return true;
}

bool Board::IsAttackedByAny(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::GetAttacks<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by)
           || Bitboards::GetAttacks<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by)
           || Bitboards::GetAttacks<KING>(s) & GetPieces(KING, attacked_by)
           || Bitboards::GetAttacks<BISHOP>(s, occ) & (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by))
           || Bitboards::GetAttacks<ROOK>(s, occ) & (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by));
}

Bitboard Board::AttackedBy(Square s, Color attacked_by, Bitboard occ) const {
    return (Bitboards::GetAttacks<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by))
           | (Bitboards::GetAttacks<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by))
           | (Bitboards::GetAttacks<KING>(s) & GetPieces(KING, attacked_by))
           | (Bitboards::GetAttacks<BISHOP>(s, occ) & (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by)))
           | (Bitboards::GetAttacks<ROOK>(s, occ) & (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by)));
}

bool Board::IsAttackedBySliders(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::GetAttacks<BISHOP>(s, occ) & (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by))
           || Bitboards::GetAttacks<ROOK>(s, occ) & (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by));
}

void Board::RemovePiece(Square s) {
    Piece p = board[s];
    board[s] = NO_PIECE;
    Bitboard pos = SqToBB(s);
    color_bbs[ColorOfPiece(p)] ^= pos;
    type_bbs[TypeOfPiece(p)] ^= pos;
    type_bbs[ALL_TYPES] ^= pos;
}

void Board::AddPiece(Square s, Piece p) {
    board[s] = p;
    Bitboard pos = SqToBB(s);
    color_bbs[ColorOfPiece(p)] |= pos;
    type_bbs[TypeOfPiece(p)] |= pos;
    type_bbs[ALL_TYPES] |= pos;
}

void Board::MovePiece(Square from, Square to) {
    Piece p = board[from];
    board[from] = NO_PIECE;
    board[to] = p;
    Bitboard from_to = SqToBB(from) ^ SqToBB(to);
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
    IncrementPly();
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
    Piece moved_piece = GetPieceOnSquare(from);
    PieceType moved_pt = TypeOfPiece(moved_piece);

    // in chess960, when castling, we remove the rook from its square before moving, and put it back later
    if (chess960 && move_type == CASTLING) {
        Move rook_move = RookCastlingMove(to, this_col);
        Square rook_from = FromSquare(rook_move);
        RemovePiece(rook_from);
    } else {
        Square capture_square = move_type == EN_PASSANT ? (this_col == WHITE ? to + SOUTH : to + NORTH) : to;
        Piece captured_piece = GetPieceOnSquare(capture_square);
        if (captured_piece) {
            RemovePiece(capture_square);
            Zobrist::RemovePiece(state.hash, captured_piece, capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        }
    }

    MovePiece(from, to);
    Zobrist::MovePiece(state.hash, moved_piece, from, to);

    Bitboard previous_cr = state.cr;
    state.cr &= ~(SqToBB(from) | SqToBB(to));
    Zobrist::UpdateCr(state.hash, previous_cr, GetCr());

    if (moved_pt == PAWN) {
        ResetPly();
        if (move_type == TWO_FORWARD) {
            SetEpSquare(this_col == WHITE ? to + SOUTH : to + NORTH);
            Zobrist::AddEp(state.hash, EpSquare());
        } else if (IsPromotion(m)) {
            Piece promoted_to = NewPiece(PromotionTo(move_type), this_col);
            RemovePiece(to);
            Zobrist::RemovePiece(state.hash, moved_piece, to);
            AddPiece(to, promoted_to);
            Zobrist::AddPiece(state.hash, promoted_to, to);
        } else if (move_type == EN_PASSANT) {
            if (IsAttackedBySliders(Bitboards::Lsb(GetPieces(KING, this_col)), next_col, GetPieces(ALL_TYPES))) {
                return false;
            }
        }
    } else if (moved_pt == KING) {
        state.cr &= Bitboards::castling_mask[next_col];
        Zobrist::UpdateCr(state.hash, previous_cr, GetCr());
        if (move_type == CASTLING) {
            Move r_move = RookCastlingMove(to, this_col);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            // put back the rook that we took out earlier in chess 960 castling
            chess960 ? AddPiece(r_to, NewPiece(ROOK, this_col)) : MovePiece(r_from, r_to);
            Zobrist::MovePiece(state.hash, NewPiece(ROOK, this_col), r_from, r_to);
            Bitboard king_walk = Bitboards::GetRayToSquares(from, to) | SqToBB(to);
            if (!AllSquaresSafe(king_walk, next_col, GetPieces(ALL_TYPES))) {
                return false;
            }
        } else if (IsAttackedByAny(to, next_col, GetPieces(ALL_TYPES))) {
            return false;
        }
    }

    Square king_s = Bitboards::Lsb(GetPieces(KING, next_col));
    state.checkers = AttackedBy(king_s, this_col, GetPieces(ALL_TYPES));

    return true;
}

void Board::UnmakeMove(Move m) {

    Square from = FromSquare(m);
    Square to = ToSquare(m);
    Piece captured_piece = CapturedPiece();

    state = history[--history_cnt];

    // ches960 castling is a special case
    if (chess960 && GetMoveType(m) == CASTLING) {
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
        if (history[i].hash == state.hash && (i > static_cast<int>(uci_moves_cnt) || ++rep > 1)) {
            return true;
        }
    }
    return false;
}

bool Board::DrawByMaterial() const {
    return (GetPieces(PAWN) | GetPieces(ROOK) | GetPieces(QUEEN)) == EMPTY_BB
           && (!Bitboards::MoreThanOne(GetPieces(WHITE)) || !Bitboards::MoreThanOne(GetPieces(BLACK)))
           && (!Bitboards::MoreThanOne(GetPieces(KNIGHT) | GetPieces(BISHOP))
               || (GetPieces(BISHOP) == EMPTY_BB && std::popcount(GetPieces(KNIGHT)) <= 2));
}

Move Board::RookCastlingMove(Square king_to, Color c) const {
    CastlingSide side = SqToFile(king_to) == FILE_G ? SHORT : LONG;
    Square from = Bitboards::Lsb(origin_rooks[c][side]);
    Square to = side == SHORT ? king_to + WEST : king_to + EAST;
    return NewMove(from, to);
}

bool Board::ParseFen(const std::string &fen) {

    static const std::regex rgx(
            R"(\s*([rnbqkpRNBQKP1-8]{1,8}\/){7}([rnbqkpRNBQKP1-8]{1,8})\s*[bw]\s*(([a-hkqA-HKQ]{1,4})|(-))?\s*(([a-h][36])|(-))?\s*\d*\s*\d*\s*)",
            std::regex::optimize);
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
            if (SqToFile(s) != FILE_A) {
                return false;
            }
            s -= 16;
        } else {
            AddPiece(s, CharToPiece(c));
            s += 1;
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
            Bitboard rooks = GetPieces(ROOK, col) & Bitboards::castling_mask[col];
            c = std::tolower(c, std::locale());
            if (c == 'k' || c == 'q' || ('a' <= c && c <= 'h')) {
                Bitboard r_bb = SqToBB(c == 'k' ? Bitboards::Msb(rooks) :
                                       c == 'q' ? Bitboards::Lsb(rooks) :
                                       FiRaToSq(CharToFile(c), col == WHITE ? RANK_1 : RANK_8));
                state.cr |= r_bb & rooks;
                origin_rooks[col][r_bb > GetPieces(KING, col) ? SHORT : LONG] = r_bb;
            }
        }
        if (std::popcount(state.cr) != static_cast<int>(token.length())) {
            return false;
        }
    }

    if (iss >> token && token != "-") {
        SetEpSquare(FiRaToSq(CharToFile(token[0]), CharToRank(token[1])));
    }

    iss >> state.ply;
    iss >> state.moves;

    return true;
}

bool Board::MakeUciMove(std::string_view move_string) {

    Move uci_move = MoveFromName(move_string);
    MoveGen move_gen(*this);

    while (Move move = move_gen.GetAnyMove()) {
        if (FromSquare(move) == FromSquare(uci_move) && ToSquare(move) == ToSquare(uci_move)) {
            if ((IsPromotion(uci_move) || GetMoveType(uci_move) == CASTLING) && move != uci_move) {
                continue;
            }
            MakeMove(move);
            ++uci_moves_cnt;
            return true;
        }
    }
    return false;
}

std::string Board::MoveToName(Move m) const {

    if (m == ZERO_MOVE) {
        return "0000";
    }

    std::string ret = SqToStr(FromSquare(m)) + SqToStr(ToSquare(m));

    // chess 960 castling is denoted by capturing own rook
    if (chess960 && GetMoveType(m) == CASTLING) {
        Rank r = SqToRank(FromSquare(m));
        Color c = r == RANK_1 ? WHITE : BLACK;
        Move r_move = RookCastlingMove(ToSquare(m), c);
        ret = SqToStr(FromSquare(m)) + SqToStr(FromSquare(r_move));
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

    Square s_from = StrToSq(&move_name[0]);
    Square s_to = StrToSq(&move_name[2]);
    MoveType flag = NO_FLAG;

    if (move_name.length() == 5) {
        flag = move_name[4] == 'q' ? PROMOTE_QUEEN :
               move_name[4] == 'r' ? PROMOTE_ROOK :
               move_name[4] == 'b' ? PROMOTE_BISHOP :
               PROMOTE_KNIGHT;
    }

    if (chess960) {
        Piece p = GetPieceOnSquare(s_to);
        if (ColorOfPiece(p) == ColorToMove() && TypeOfPiece(p) == ROOK) {
            s_to = s_to > s_from ? ColorToMove() == WHITE ? G1 : G8 : ColorToMove() == WHITE ? C1 : C8;
            flag = CASTLING;
        }
    }

    return NewMove(s_from, s_to, flag);
}

[[maybe_unused]] std::string Board::GetFen() const {

    std::ostringstream oss;

    for (Rank r: Ranks | std::views::reverse) {
        int empty_cnt = 0;
        for (File f: Files) {
            Piece p = GetPieceOnSquare(FiRaToSq(f, r));
            if (p != NO_PIECE) {
                if (empty_cnt > 0) {
                    oss << empty_cnt;
                    empty_cnt = 0;
                }
                oss << PieceToChar(p);
            } else {
                ++empty_cnt;
            }
        }
        if (empty_cnt > 0) {
            oss << empty_cnt;
        }
        oss << (r > RANK_1 ? '/' : ' ');
    }

    oss << (ColorToMove() == WHITE ? 'w' : 'b') << ' ';

    if (!GetCr()) {
        oss << '-';
    } else {
        if (CrAvailable(WHITE, SHORT))
            oss << (chess960 ? std::toupper(FileToChar(SqToFile(RookSq(WHITE, SHORT))), std::locale()) : 'K');
        if (CrAvailable(WHITE, LONG))
            oss << (chess960 ? std::toupper(FileToChar(SqToFile(RookSq(WHITE, LONG))), std::locale()) : 'Q');
        if (CrAvailable(BLACK, SHORT))
            oss << (chess960 ? FileToChar(SqToFile(RookSq(BLACK, SHORT))) : 'k');
        if (CrAvailable(BLACK, LONG))
            oss << (chess960 ? FileToChar(SqToFile(RookSq(BLACK, LONG))) : 'q');
    }

    oss << ' ' << (EpSquare() ? SqToStr(EpSquare()) : "-") << ' ' << Ply() << ' ' << FullMoveClock();

    return oss.str();
}

[[maybe_unused]] std::string Board::PrettyPrint() const {

    std::ostringstream oss;

    for (Rank r: Ranks | std::views::reverse) {
        oss << r + 1 << " |";
        for (File f: Files) {
            oss << ' ' << PieceToChar(GetPieceOnSquare(FiRaToSq(f, r))) << (f == FILE_H ? "" : " ");
        }
        oss << '\n';
    }
    oss << "---------------------------\n"
        << "  | A  B  C  D  E  F  G  H\n\n"
        << "FEN: " << GetFen() << '\n'
        << "Hash: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << GetHash();
    if (chess960) {
        oss << "\nChess 960 board";
    }

    return oss.str();
}
