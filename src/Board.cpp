#include <sstream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <bit>
#include <ranges>
#include "Board.h"
#include "MoveGen.h"
#include "ZobristHash.h"

Board::Board() {
    NewPosition(STARTPOS_FEN.data());
}

bool Board::NewPosition(const std::string &fen, bool isChess960) {

    Board previous = *this;

    chess960 = isChess960;
    history_cnt = 0;
    uci_moves_cnt = 0;
    state = BoardState();
    board.fill(NO_PIECE);
    color_bbs.fill(EMPTY_BB);
    type_bbs.fill(EMPTY_BB);
    origin_rooks[BLACK].fill(EMPTY_BB);
    origin_rooks[WHITE].fill(EMPTY_BB);

    if (!ParseFen(fen) || !IsValid()) {
        *this = previous;
        return false;
    }

    state.hash = Zobrist::GenHash64(*this);
    state.evaluator.SetBoard(*this);

    Square king_s = Bitboards::Lsb(Pieces(KING, ColorToMove()));
    state.checkers = AttackedBy(king_s, OtherColor(ColorToMove()), Pieces(ALL_TYPES));

    return true;
}

bool Board::IsValid() const {

    Bitboard white_king = Pieces(KING, WHITE);
    Bitboard black_king = Pieces(KING, BLACK);
    if (!Bitboards::ExactlyOne(white_king) || !Bitboards::ExactlyOne(black_king)) {
        return false;
    }

    Square enemy_king_square = ColorToMove() == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
    if (IsAttackedByAny(enemy_king_square, ColorToMove(), Pieces(ALL_TYPES))) {
        return false;
    }

    if (IsCastlingAvailable(WHITE, SHORT) || IsCastlingAvailable(WHITE, LONG)) {
        if (!(white_king & Bitboards::castling_mask[WHITE])) {
            return false;
        }
        if (!chess960 && Bitboards::Lsb(white_king) != E1) {
            return false;
        }
    }

    if (IsCastlingAvailable(BLACK, SHORT) || IsCastlingAvailable(BLACK, LONG)) {
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
        if (PieceOnSquare(capture_s) != NewPiece(PAWN, OtherColor(ColorToMove()))
            || !(rank_mask & SqToBB(capture_s))) {
            return false;
        }
    }

    if (Pieces(PAWN) & (Bitboards::rank_mask[RANK_8] | Bitboards::rank_mask[RANK_1])) {
        return false;
    }

    return true;
}

Bitboard Board::PinnedToSquare(Square s, Color blockers_color) const {

    Bitboard all_pieces = Pieces(ALL_TYPES);
    Bitboard queens = Pieces(QUEEN, blockers_color);
    Bitboard bishops = Pieces(BISHOP, blockers_color);
    Bitboard rooks = Pieces(ROOK, blockers_color);

    Bitboard attackers = ((bishops | queens) & Bitboards::BishopRays(s))
                         | ((rooks | queens) & Bitboards::RookRays(s));

    Bitboard pinned_pieces = EMPTY_BB;
    while (attackers) {
        Square attacker_s = Bitboards::PopLsb(attackers);
        Bitboard blocker = Bitboards::RayToSquares(attacker_s, s) & all_pieces;
        if (Bitboards::ExactlyOne(blocker)) {
            pinned_pieces |= blocker;
        }
    }

    return pinned_pieces;
}

// takes a pseudo legal move and checks whether it is legal
bool Board::IsMoveLegal(Move m) const {

    if (PieceTypeOnSq(FromSquare(m)) == KING) {
        if (TypeOfMove(m) == CASTLING) {
            Square from = FromSquare(m);
            Square to = ToSquare(m);
            Move r_move = RookCastlingMove(to, ColorToMove());
            // the xor is for chess960, when king doesn't move, but the rook moving could open an attack on the king
            Bitboard occ = Pieces(ALL_TYPES) ^ SqToBB(FromSquare(r_move));
            Bitboard king_walk = Bitboards::RayToSquares(from, to) | SqToBB(to);
            return AllSquaresSafe(king_walk, OtherColor(ColorToMove()), occ);
        }
        Bitboard occ = Pieces(ALL_TYPES) ^ SqToBB(FromSquare(m));
        return !IsAttackedByAny(ToSquare(m), OtherColor(ColorToMove()), occ);
    }

    if (TypeOfMove(m) == EN_PASSANT) {
        Color my_col = ColorToMove();
        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Square capture_s = my_col == WHITE ? to + SOUTH : to + NORTH;
        Square king_s = Bitboards::Lsb(Pieces(KING, my_col));
        Bitboard occ = Pieces(ALL_TYPES) ^ SqToBB(capture_s) ^ (SqToBB(from) | SqToBB(to));
        // ep move can't open us to attacks by anything other than slider pieces
        return !IsAttackedBySliders(king_s, OtherColor(my_col), occ);
    }

    return true;
}

bool Board::IsAttackedByAny(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::Attacks<KNIGHT>(s) & Pieces(KNIGHT, attacked_by)
           || Bitboards::Attacks<PAWN>(s, occ, OtherColor(attacked_by)) & Pieces(PAWN, attacked_by)
           || Bitboards::Attacks<KING>(s) & Pieces(KING, attacked_by)
           || Bitboards::Attacks<BISHOP>(s, occ) & (Pieces(BISHOP, attacked_by) | Pieces(QUEEN, attacked_by))
           || Bitboards::Attacks<ROOK>(s, occ) & (Pieces(ROOK, attacked_by) | Pieces(QUEEN, attacked_by));
}

Bitboard Board::AttackedBy(Square s, Color attacked_by, Bitboard occ) const {
    return (Bitboards::Attacks<KNIGHT>(s) & Pieces(KNIGHT, attacked_by))
           | (Bitboards::Attacks<PAWN>(s, occ, OtherColor(attacked_by)) & Pieces(PAWN, attacked_by))
           | (Bitboards::Attacks<KING>(s) & Pieces(KING, attacked_by))
           | (Bitboards::Attacks<BISHOP>(s, occ) & (Pieces(BISHOP, attacked_by) | Pieces(QUEEN, attacked_by)))
           | (Bitboards::Attacks<ROOK>(s, occ) & (Pieces(ROOK, attacked_by) | Pieces(QUEEN, attacked_by)));
}

bool Board::IsAttackedBySliders(Square s, Color attacked_by, Bitboard occ) const {
    return Bitboards::Attacks<BISHOP>(s, occ) & (Pieces(BISHOP, attacked_by) | Pieces(QUEEN, attacked_by))
           || Bitboards::Attacks<ROOK>(s, occ) & (Pieces(ROOK, attacked_by) | Pieces(QUEEN, attacked_by));
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
    MoveType move_type = TypeOfMove(m);
    Piece moved_piece = PieceOnSquare(from);
    PieceType moved_pt = TypeOfPiece(moved_piece);

    // in chess960, when castling, we remove the rook from its square before moving, and put it back later
    if (chess960 && move_type == CASTLING) {
        Move rook_move = RookCastlingMove(to, this_col);
        RemovePiece(FromSquare(rook_move));
    } else {
        Square capture_square = move_type == EN_PASSANT ? EpCaptureSq(this_col, to) : to;
        Piece captured_piece = PieceOnSquare(capture_square);
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
    Zobrist::UpdateCr(state.hash, previous_cr, Cr());

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
            if (IsAttackedBySliders(Bitboards::Lsb(Pieces(KING, this_col)), next_col, Pieces(ALL_TYPES))) {
                return false;
            }
        }
    } else if (moved_pt == KING) {
        state.cr &= Bitboards::castling_mask[next_col];
        Zobrist::UpdateCr(state.hash, previous_cr, Cr());
        if (move_type == CASTLING) {
            Move r_move = RookCastlingMove(to, this_col);
            Square r_to = ToSquare(r_move);
            Square r_from = FromSquare(r_move);
            // put back the rook that we took out earlier in chess 960 castling
            chess960 ? AddPiece(r_to, NewPiece(ROOK, this_col)) : MovePiece(r_from, r_to);
            Zobrist::MovePiece(state.hash, NewPiece(ROOK, this_col), r_from, r_to);
            Bitboard king_walk = Bitboards::RayToSquares(from, to) | SqToBB(to);
            if (!AllSquaresSafe(king_walk, next_col, Pieces(ALL_TYPES))) {
                return false;
            }
        } else if (IsAttackedByAny(to, next_col, Pieces(ALL_TYPES))) {
            return false;
        }
    }

    Square king_s = Bitboards::Lsb(Pieces(KING, next_col));
    state.checkers = AttackedBy(king_s, this_col, Pieces(ALL_TYPES));

    return true;
}

void Board::UnmakeMove(Move m) {

    Square from = FromSquare(m);
    Square to = ToSquare(m);
    Piece captured_piece = CapturedPiece();

    state = history[--history_cnt];

    if (chess960 && TypeOfMove(m) == CASTLING) {
        Move rook_move = RookCastlingMove(to, ColorToMove());
        RemovePiece(ToSquare(rook_move));
        MovePiece(to, from);
        AddPiece(FromSquare(rook_move), NewPiece(ROOK, ColorToMove()));
        return;
    }

    MovePiece(to, from);

    if (captured_piece) {
        Square capture_s = TypeOfMove(m) == EN_PASSANT ? EpCaptureSq(ColorToMove(), to) : to;
        AddPiece(capture_s, captured_piece);
    }

    if (IsPromotion(m)) {
        RemovePiece(from);
        AddPiece(from, NewPiece(PAWN, ColorToMove()));
    } else if (TypeOfMove(m) == CASTLING) {
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

bool Board::IsDraw() const {
    return (IsMove50Rule() && !(IsInCheck() && !AnyLegalMoves())) // could be checkmated on the 50th move
           || IsRepetition()
           || (!IsInCheck() && IsDrawByMaterial());
}

bool Board::IsRepetition() const {
    int stop = std::max(HistorySize() - Ply(), 0);
    for (int i = HistorySize() - 2, rep = 0; i >= stop; i -= 2) {
        if (history[i].hash == state.hash && (i > uci_moves_cnt || ++rep > 1)) {
            return true;
        }
    }
    return false;
}

bool Board::IsDrawByMaterial() const {
    return (Pieces(PAWN) | Pieces(ROOK) | Pieces(QUEEN)) == EMPTY_BB
           && (!Bitboards::MoreThanOne(Pieces(WHITE)) || !Bitboards::MoreThanOne(Pieces(BLACK)))
           && (!Bitboards::MoreThanOne(Pieces(KNIGHT) | Pieces(BISHOP))
               || (Pieces(BISHOP) == EMPTY_BB && std::popcount(Pieces(KNIGHT)) <= 2));
}

bool Board::AnyLegalMoves() const {
    MoveGen mg(*this);
    while (Move m = mg.NextMove()) {
        if (IsMoveLegal(m)) {
            return true;
        }
    }
    return false;
}

Move Board::RookCastlingMove(Square king_to, Color c) const {
    CastlingSide side = SqToFile(king_to) == FILE_G ? SHORT : LONG;
    Square from = Bitboards::Lsb(origin_rooks[c][side]);
    Square to = side == SHORT ? king_to + WEST : king_to + EAST;
    return NewMove(from, to);
}

bool Board::ParseFen(const std::string &fen) {

    static constexpr auto pattern = R"(\s*([rnbqkpRNBQKP1-8]{1,8}\/){7}([rnbqkpRNBQKP1-8]{1,8})\s*[bw]\s*(([a-hkqA-HKQ]{1,4})|(-))?\s*(([a-h][36])|(-))?\s*\d*\s*\d*\s*)";
    static const std::regex rgx(pattern, std::regex::optimize);
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
            Bitboard rooks = Pieces(ROOK, col) & Bitboards::castling_mask[col];
            c = std::tolower(c, std::locale());
            if (c == 'k' || c == 'q' || ('a' <= c && c <= 'h')) {
                Bitboard r_bb = SqToBB(c == 'k' ? Bitboards::Msb(rooks) :
                                       c == 'q' ? Bitboards::Lsb(rooks) :
                                       FiRaToSq(CharToFile(c), col == WHITE ? RANK_1 : RANK_8));
                state.cr |= r_bb & rooks;
                origin_rooks[col][r_bb > Pieces(KING, col) ? SHORT : LONG] = r_bb;
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

bool Board::MakeUciMove(std::string_view move_str) {

    static constexpr auto pattern = R"(([a-h][1-8]){2}[qrbn]?)";
    static const std::regex rgx(pattern, std::regex::optimize);
    if (!std::regex_match(move_str.data(), rgx)) {
        return false;
    }

    Move uci_move = StrToMove(move_str);
    MoveGen move_gen(*this);

    while (Move move = move_gen.NextMove()) {
        if (move == uci_move) {
            MakeMove(move);
            ++uci_moves_cnt;
            return true;
        }
    }
    return false;
}

std::string Board::MoveToStr(Move m) const {

    if (m == ZERO_MOVE) {
        return "0000";
    }

    std::string ret = SqToStr(FromSquare(m)) + SqToStr(ToSquare(m));

    // chess 960 castling is denoted by capturing own rook
    if (chess960 && TypeOfMove(m) == CASTLING) {
        Rank r = SqToRank(FromSquare(m));
        Color c = r == RANK_1 ? WHITE : BLACK;
        Move r_move = RookCastlingMove(ToSquare(m), c);
        ret = SqToStr(FromSquare(m)) + SqToStr(FromSquare(r_move));
    }

    if (IsPromotion(m)) {
        MoveType prom_flag = TypeOfMove(m);
        ret += prom_flag == PROMOTE_QUEEN ? 'q' :
               prom_flag == PROMOTE_ROOK ? 'r' :
               prom_flag == PROMOTE_BISHOP ? 'b' :
               'n';
    }

    return ret;
}

Move Board::StrToMove(std::string_view move_str) const {

    Square s_from = StrToSq(&move_str[0]);
    Square s_to = StrToSq(&move_str[2]);
    MoveType flag = NO_FLAG;
    PieceType moved_pt = PieceTypeOnSq(s_from);

    if (move_str.length() == 5) {
        flag = move_str[4] == 'q' ? PROMOTE_QUEEN :
               move_str[4] == 'r' ? PROMOTE_ROOK :
               move_str[4] == 'b' ? PROMOTE_BISHOP :
               PROMOTE_KNIGHT;
    } else if (chess960 && PieceTypeOnSq(s_to) == ROOK && ColorOfPiece(PieceOnSquare(s_to)) == ColorToMove()) {
        // FRC castling is denoted as capturing own rook, but internally we use to_square as the real square to move to
        s_to = s_to > s_from ? ColorToMove() == WHITE ? G1 : G8 : ColorToMove() == WHITE ? C1 : C8;
        flag = CASTLING;
    } else if (moved_pt == KING && Bitboards::ExactlyOne(Bitboards::RayToSquares(s_from, s_to))) {
        flag = CASTLING;
    } else if (moved_pt == PAWN && Bitboards::ExactlyOne(Bitboards::RayToSquares(s_from, s_to))) {
        flag = TWO_FORWARD;
    } else if (moved_pt == PAWN && s_to == EpSquare()) {
        flag = EN_PASSANT;
    }

    return NewMove(s_from, s_to, flag);
}

std::string Board::Fen() const {

    std::ostringstream oss;

    for (Rank r: Ranks | std::views::reverse) {
        int empty_cnt = 0;
        for (File f: Files) {
            Piece p = PieceOnSquare(FiRaToSq(f, r));
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

    if (!Cr()) {
        oss << '-';
    } else {
        if (IsCastlingAvailable(WHITE, SHORT))
            oss << (chess960 ? std::toupper(FileToChar(SqToFile(RookSq(WHITE, SHORT))), std::locale()) : 'K');
        if (IsCastlingAvailable(WHITE, LONG))
            oss << (chess960 ? std::toupper(FileToChar(SqToFile(RookSq(WHITE, LONG))), std::locale()) : 'Q');
        if (IsCastlingAvailable(BLACK, SHORT))
            oss << (chess960 ? FileToChar(SqToFile(RookSq(BLACK, SHORT))) : 'k');
        if (IsCastlingAvailable(BLACK, LONG))
            oss << (chess960 ? FileToChar(SqToFile(RookSq(BLACK, LONG))) : 'q');
    }

    oss << ' ' << (EpSquare() ? SqToStr(EpSquare()) : "-") << ' ' << Ply() << ' ' << FullMoveClock();

    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const Board &b) {

    for (Rank r: Ranks | std::views::reverse) {
        os << r + 1 << " |";
        for (File f: Files) {
            os << ' ' << PieceToChar(b.PieceOnSquare(FiRaToSq(f, r))) << (f == FILE_H ? "" : " ");
        }
        os << '\n';
    }
    os << "---------------------------\n"
       << "  | A  B  C  D  E  F  G  H\n\n"
       << "FEN: " << b.Fen() << '\n'
       << "Hash: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << b.Hash();
    if (b.chess960) {
        os << "\nChess 960 board";
    }

    return os;
}
