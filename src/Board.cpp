#include "Board.h"
#include "Bitboards.h"
#include <sstream>
#include "Uci.h"
#include "Utils.h"
#include "MoveGen.h"
#include <algorithm>
#include "Search.h"

namespace Meetra {

    Board::Board() {
        NewPosition(STARTPOS_FEN);
    }

    bool Board::NewPosition(const std::string &fen) {

        Board previous = *this;

        history_cnt = 0;
        curr_data.state = NEW_GAME_STATE;
        curr_data.cr = EMPTY_BB;
        std::ranges::fill(board, NO_PIECE);
        std::ranges::fill(color_bbs, EMPTY_BB);
        std::ranges::fill(type_bbs, EMPTY_BB);
        K_rook = EMPTY_BB;
        Q_rook = EMPTY_BB;
        k_rook = EMPTY_BB;
        q_rook = EMPTY_BB;

        if (!ParseFenValidate(fen) || !IsBoardValid()) {
            *this = previous;
            return false;
        }

        curr_data.hash = Zobrist::GenHash(*this);
        curr_data.evaluator.SetBoard(*this);

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

        Color now_move = ColorToMove();
        Square enemy_king_square = now_move == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
        if (IsAttackedByAny(enemy_king_square, now_move, GetPieces(ALL_TYPES))) {
            return false;
        }

        return true;
    }

    Bitboard Board::PinnedPiecesForSquare(Square s, Color attackers_color) const {

        Bitboard all_pieces = GetPieces(ALL_TYPES);
        Bitboard queens = GetPieces(QUEEN, attackers_color);
        Bitboard bishops = GetPieces(BISHOP, attackers_color);
        Bitboard rooks = GetPieces(ROOK, attackers_color);

        Bitboard attackers = ((bishops | queens) & Bitboards::GetUnboundBishopMoves(s)) |
                             ((rooks | queens) & Bitboards::GetUnboundRookMoves(s));

        Bitboard pinned_pieces = EMPTY_BB;
        while (attackers) {
            Square attacker_s = Bitboards::PopLsb(attackers);
            Bitboard blocker = Bitboards::GetRayBetweenSquares(attacker_s, s) & all_pieces;
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
                Square to = ToSquare(m);
                Square from = FromSquare(m);
                Move r_move = RookCastlingMove(to, GetCr(), ColorToMove());
                Square r_from = FromSquare(r_move);
                // the xor is for chess960, when king doesn't, but the rook moving would open an attack on the king
                Bitboard occ = GetPieces(ALL_TYPES) ^ SquareToBB(r_from);
                Bitboard king_walk = Bitboards::GetRayBetweenSquares(from, to) | SquareToBB(to);
                return AllSquaresSafe(king_walk, OtherColor(ColorToMove()), occ);
            }
            Bitboard occ = GetPieces(ALL_TYPES) ^ SquareToBB(FromSquare(m));
            return !IsAttackedByAny(ToSquare(m), OtherColor(ColorToMove()), occ);
        }

        if (GetMoveType(m) == EN_PASSANT) {
            Color my_color = ColorToMove();
            Square from = FromSquare(m);
            Square to = ToSquare(m);
            Square capture_square = my_color == WHITE ? to + SOUTH : to + NORTH;
            Square king_s = Bitboards::Lsb(GetPieces(KING, my_color));
            Bitboard occ = GetPieces(ALL_TYPES) ^ SquareToBB(capture_square) ^ (SquareToBB(from) | SquareToBB(to));
            // ep move can't open us to attacks by anything other than slider pieces
            return !IsAttackedBySliders(king_s, OtherColor(my_color), occ);
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

    void Board::PutPiece(Square s, Piece p) {
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

    // Make a pseudo legal move.
    // If the move turns out to be illegal, it has to be undone via the UnmakeMove function.
    // It is better to assume all attempted moves are legal, rather than first verifying the legality and only then
    // making them, because only a very few moves generated by our move gen actually turn out to be illegal.
    bool Board::MakeMove(Move m) {

        history[history_cnt++] = curr_data;

        curr_data.evaluator.MakeMove(*this, m);

        Color this_col = ColorToMove();
        ChangeColorToMove();
        Color next_col = ColorToMove();
        Zobrist::UpdateColor(curr_data.hash, next_col);

        IncrementMoveNumber(this_col);
        IncrementPly();
        ClearCapturedPiece();
        if (EpSquare()) {
            Zobrist::RemoveEp(curr_data.hash, EpSquare());
            ClearEpSquare();
        }

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        MoveType move_type = GetMoveType(m);

        Piece captured_piece = move_type == EN_PASSANT ? NewPiece(PAWN, next_col) : GetPieceOnSquare(to);

        // in chess960, when castling, we remove the rook from its square before moving, and put it back later
        // we also need to set captured piece to none, in case the king doesn't move from its square
        if (Search::Globals::chess960 && move_type == CASTLING) {
            captured_piece = NO_PIECE;
            Move rook_move = RookCastlingMove(to, GetCr(), this_col);
            Square rook_from = FromSquare(rook_move);
            RemovePiece(rook_from);
        }

        if (captured_piece) {
            Square capture_square = move_type == EN_PASSANT ? (this_col == WHITE ? to + SOUTH : to + NORTH) : to;
            RemovePiece(capture_square);
            Zobrist::RemovePiece(curr_data.hash, captured_piece, capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        }

        Piece moved_piece = GetPieceOnSquare(from);
        MovePiece(from, to);
        Zobrist::MovePiece(curr_data.hash, moved_piece, from, to);

        Bitboard previous_cr = curr_data.cr;
        curr_data.cr &= ~(SquareToBB(from) | SquareToBB(to));
        Zobrist::UpdateCr(curr_data.hash, previous_cr, GetCr());

        if (TypeOfPiece(moved_piece) == PAWN) {
            ResetPly();
            if (move_type == TWO_FORWARD) {
                SetEpSquare(this_col == WHITE ? to + SOUTH : to + NORTH);
                Zobrist::AddEp(curr_data.hash, EpSquare());
                return true;
            } else if (IsPromotion(m)) {
                Piece promoted_to = NewPiece(PieceTypeFromFlag(move_type), this_col);
                RemovePiece(to);
                Zobrist::RemovePiece(curr_data.hash, moved_piece, to);
                PutPiece(to, promoted_to);
                Zobrist::PutPiece(curr_data.hash, promoted_to, to);
                return true;
            } else if (move_type == EN_PASSANT) {
                return !IsAttackedBySliders(Bitboards::Lsb(GetPieces(KING, this_col)), next_col,
                                            GetPieces(ALL_TYPES));
            }
            return true;
        }

        if (TypeOfPiece(moved_piece) == KING) {

            curr_data.cr &= this_col == WHITE ? Bitboards::GetRankMask(RANK_8) : Bitboards::GetRankMask(RANK_1);
            Zobrist::UpdateCr(curr_data.hash, previous_cr, GetCr());

            if (move_type == CASTLING) {
                Move r_move = RookCastlingMove(to, previous_cr, this_col);
                Square r_to = ToSquare(r_move);
                Square r_from = FromSquare(r_move);
                // put back the rook in chess 960 castling
                Search::Globals::chess960 ? PutPiece(r_to, NewPiece(ROOK, this_col)) : MovePiece(r_from, r_to);
                Zobrist::MovePiece(curr_data.hash, NewPiece(ROOK, this_col), r_from, r_to);

                Bitboard king_walk = Bitboards::GetRayBetweenSquares(from, to) | SquareToBB(to);
                return AllSquaresSafe(king_walk, next_col, GetPieces(ALL_TYPES));
            }

            return !IsAttackedByAny(to, next_col, GetPieces(ALL_TYPES));
        }

        return true;
    }

    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Piece captured_piece = CapturedPiece();

        curr_data = history[--history_cnt];

        // ches960 castling, special case
        if (Search::Globals::chess960 && GetMoveType(m) == CASTLING) {
            Move rook_move = RookCastlingMove(to, GetCr(), ColorToMove());
            RemovePiece(ToSquare(rook_move));
            MovePiece(to, from);
            PutPiece(FromSquare(rook_move), NewPiece(ROOK, ColorToMove()));
            return;
        }

        MovePiece(to, from);

        if (captured_piece) {
            Square capture_s = GetMoveType(m) == EN_PASSANT ? ColorToMove() == WHITE ? to + SOUTH : to + NORTH : to;
            PutPiece(capture_s, captured_piece);
        }

        if (IsPromotion(m)) {
            RemovePiece(from);
            PutPiece(from, NewPiece(PAWN, ColorToMove()));
        } else if (GetMoveType(m) == CASTLING) {
            Move rook_move = RookCastlingMove(to, GetCr(), ColorToMove());
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

    Move Board::RookCastlingMove(Square king_to, Bitboard cr, Color c) const {
        cr &= c == WHITE ? Bitboards::GetRankMask(RANK_1) : Bitboards::GetRankMask(RANK_8);
        Square to = FileFromSquare(king_to) == FILE_G ? king_to - 1 : king_to + 1;
        Square from = FileFromSquare(to) == FILE_F ? Bitboards::Msb(cr) : Bitboards::Lsb(cr);
        return NewMove(from, to);
    }

    bool Board::MakeUciMove(const std::string &move_string) {

        Move move_made = NewMoveFromName(move_string);
        MoveGen move_gen(*this);
        Move move;

        while ((move = move_gen.GetAnyMove())) {
            if (FromSquare(move) == FromSquare(move_made) && ToSquare(move) == ToSquare(move_made)) {
                if (IsPromotion(move) && move != move_made) {
                    continue;
                }
                MakeMove(move);
                return true;
            }
        }
        return false;
    }

    void Board::ParseFen(const std::string &fen) {

        std::istringstream iss(fen);
        std::string token;

        iss >> token;
        Square s = A8;
        for (char c: token) {
            if (std::isdigit(c)) {
                s += c - '0';
            } else if (c == '/') {
                s -= 16;
            } else {
                PutPiece(s, CharToPiece(c));
                ++s;
            }
        }

        iss >> token;
        SetColorToMove(token == "w" ? WHITE : BLACK);

        iss >> token;
        if (token.find('K') != std::string::npos) curr_data.cr |= SquareToBB(H1);
        if (token.find('Q') != std::string::npos) curr_data.cr |= SquareToBB(A1);
        if (token.find('k') != std::string::npos) curr_data.cr |= SquareToBB(H8);
        if (token.find('q') != std::string::npos) curr_data.cr |= SquareToBB(A8);
        for (char c: token) {
            if ('A' <= c && c <= 'H') curr_data.cr |= SquareToBB(SqFromFiRa(FileFromChar(tolower(c)), RANK_1));
            if ('a' <= c && c <= 'h') curr_data.cr |= SquareToBB(SqFromFiRa(FileFromChar(c), RANK_8));
        }

        iss >> token;
        if (token != "-") {
            File file = FileFromChar(token[0]);
            Rank rank = RankFromChar(token[1]);
            SetEpSquare(SqFromFiRa(file, rank));
        }

        iss >> token;
        SetPly(std::stoi(token));

        iss >> token;
        SetMoveNumber(std::stoi(token));
    }

    bool Board::ParseFenValidate(const std::string &fen) {

        std::istringstream iss(fen);
        std::string token;

        // board position
        iss >> token;
        File f = FILE_A;
        Rank r = RANK_8;
        bool prev_is_digit = false;
        for (char c: token) {

            if ((c != '/' && f == FILE_H + 1) || (c == '/' && f <= FILE_H) || (prev_is_digit && std::isdigit(c)) ||
                r < RANK_1 || f > FILE_H + 1) {
                return false;
            }

            if (c == '/' && f == FILE_H + 1) {
                prev_is_digit = false;
                f = FILE_A;
                --r;
            } else if (c <= '8' && c >= '1') {
                prev_is_digit = true;
                f += c - '0';
            } else if (CharToPiece(c) != NO_PIECE) {
                prev_is_digit = false;
                PutPiece(SqFromFiRa(f, r), CharToPiece(c));
                ++f;
            } else {
                return false;
            }
        }

        // color to move
        if (iss >> token && (token == "w" || token == "b")) {
            SetColorToMove(token == "w" ? WHITE : BLACK);
        } else {
            return false;
        }

        // castling rights
        if (iss >> token) {
            if (Utils::ContainsOnlyChars(token, "KQkqABCDEFGHabcdefgh") && Utils::AllUniqueChars(token)) {

                Bitboard w_king = GetPieces(KING, WHITE);
                Bitboard b_king = GetPieces(KING, BLACK);
                Bitboard w_rooks = GetPieces(ROOK, WHITE) & Bitboards::GetRankMask(RANK_1);
                Bitboard b_rooks = GetPieces(ROOK, BLACK) & Bitboards::GetRankMask(RANK_8);

                for(char c : token) {

                    if (c == 'K') {
                        Bitboard r_square = SquareToBB(Bitboards::Msb(w_rooks));
                        curr_data.cr |= r_square;
                        K_rook = r_square;
                    }
                    if (c == 'Q') {
                        Bitboard r_square = SquareToBB(Bitboards::Lsb(w_rooks));
                        curr_data.cr |= r_square;
                        Q_rook = r_square;
                    }
                    if (c == 'k') {
                        Bitboard r_square = SquareToBB(Bitboards::Msb(b_rooks));
                        curr_data.cr |= r_square;
                        k_rook = r_square;
                    }
                    if (c == 'q') {
                        Bitboard r_square = SquareToBB(Bitboards::Lsb(b_rooks));
                        curr_data.cr |= r_square;
                        q_rook = r_square;
                    }
                    if ('A' <= c && c <= 'H') {
                        Bitboard r_square = SquareToBB(SqFromFiRa(FileFromChar(tolower(c)), RANK_1));
                        curr_data.cr |= r_square;
                        if (r_square > w_king) {
                            K_rook = r_square;
                        } else {
                            Q_rook = r_square;
                        }
                    }
                    if ('a' <= c && c <= 'h') {
                        Bitboard r_square = SquareToBB(SqFromFiRa(FileFromChar(c), RANK_8));
                        curr_data.cr |= r_square;
                        if (r_square > b_king) {
                            k_rook = r_square;
                        } else {
                            q_rook = r_square;
                        }
                    }
                }

                if ((curr_data.cr & GetPieces(ROOK)) != curr_data.cr) {
                    return false;
                }

            } else if (token != "-") {
                return false;
            }
        }

        // en passant square
        if (iss >> token) {
            if (token.length() == 2 && token[0] >= 'a' && token[0] <= 'h' && token[1] >= '1' && token[1] <= '8') {
                File file = FileFromChar(token[0]);
                Rank rank = RankFromChar(token[1]);
                SetEpSquare(SqFromFiRa(file, rank));
            } else if (token != "-") {
                return false;
            }
        }

        // ply
        if (iss >> token) {
            if (Utils::IsPositiveNumber(token) && std::stoi(token) < 256) {
                SetPly(std::stoi(token));
            } else {
                return false;
            }
        }

        // move count
        if (iss >> token) {
            if (Utils::IsPositiveNumber(token) && std::stoi(token) < MAX_GAME_LENGTH) {
                SetMoveNumber(std::stoi(token));
            } else {
                return false;
            }
        } else {
            SetMoveNumber(1);
        }

        return true;
    }

    std::string Board::GetMoveName(Move m) const {

        if (m == ZERO_MOVE) {
            return "0000";
        }

        std::string ret = SquareToName(FromSquare(m)) + SquareToName(ToSquare(m));

        // chess 960 castling is denoted by capturing own rook
        if(Search::Globals::chess960 && GetMoveType(m) == CASTLING) {
            Move r_move = RookCastlingMove(ToSquare(m), GetCr(), ColorToMove());
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

    std::string Board::PPBoard() const {

        std::ostringstream oss;

        for (Rank r = RANK_8; r >= RANK_1; --r) {
            oss << std::to_string(r + 1) << " |";
            for (File f = FILE_A; f <= FILE_H; ++f) {
                oss << ' ' << PieceToChar(GetPieceOnSquare(SqFromFiRa(f, r))) << ' ';
            }
            oss << '\n';
        }
        oss << "---------------------------\n"
            << "  | A  B  C  D  E  F  G  H\n\n"
            << "Player to move: " << (ColorToMove() == WHITE ? "white\n" : "black\n")
            << "Castling rights: ";
        if (!CanCastleAny()) {
            oss << '-';
        } else {
            if (CanWShortCastle()) oss << 'K';
            if (CanWLongCastle()) oss << 'Q';
            if (CanBShortCastle()) oss << 'k';
            if (CanBLongCastle()) oss << 'q';
        }
        oss << " | EP square: " << (EpSquare() == SQUARE_ZERO ? "-" : SquareToName(EpSquare())) << '\n'
            << "Fullmove clock: " << TotalMoves() << " | Halfmove clock: " << Ply();

        return oss.str();
    }


}
