#include "Board.h"
#include "Bitboards.h"
#include <sstream>
#include "Uci.h"
#include "Utils.h"
#include "MoveGen.h"
#include <algorithm>
#include "Search.h"
#include <regex>

namespace Meetra {

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
        std::ranges::fill(orig_rooks[BLACK], EMPTY_BB);
        std::ranges::fill(orig_rooks[WHITE], EMPTY_BB);

        if (!ParseFenValidate(fen) || !IsBoardValid()) {
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

        Color now_move = ColorToMove();
        Square enemy_king_square = now_move == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
        if (IsAttackedByAny(enemy_king_square, now_move, GetPieces(ALL_TYPES))) {
            return false;
        }

        return true;
    }

    Bitboard Board::PinnedToSquare(Square s, Color blockers_color) const {

        Bitboard all_pieces = GetPieces(ALL_TYPES);
        Bitboard queens = GetPieces(QUEEN, blockers_color);
        Bitboard bishops = GetPieces(BISHOP, blockers_color);
        Bitboard rooks = GetPieces(ROOK, blockers_color);

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
                Move r_move = RookCastlingMove(to, ColorToMove());
                Square r_from = FromSquare(r_move);
                // the xor is for chess960, when king doesn't move, but the rook moving could open an attack on the king
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

        history[history_cnt++] = state;

        state.evaluator.MakeMove(*this, m);

        Color this_col = ColorToMove();
        ChangeColorToMove();
        Color next_col = ColorToMove();
        Zobrist::UpdateColor(state.hash, next_col);

        IncrementMoveNumber(this_col);
        IncrementPly();
        ClearCapturedPiece();
        if (EpSquare()) {
            Zobrist::RemoveEp(state.hash, EpSquare());
            ClearEpSquare();
        }

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        MoveType move_type = GetMoveType(m);

        Piece captured_piece = move_type == EN_PASSANT ? NewPiece(PAWN, next_col) : GetPieceOnSquare(to);

        // in chess960, when castling, we remove the rook from its square before moving, and put it back later
        // we also need to set captured piece to none, in case the king tries to capture itself
        if (Search::Globals::chess960 && move_type == CASTLING) {
            captured_piece = NO_PIECE;
            Move rook_move = RookCastlingMove(to, this_col);
            Square rook_from = FromSquare(rook_move);
            RemovePiece(rook_from);
        }

        if (captured_piece) {
            Square capture_square = move_type == EN_PASSANT ? (this_col == WHITE ? to + SOUTH : to + NORTH) : to;
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
                return true;
            } else if (IsPromotion(m)) {
                Piece promoted_to = NewPiece(PieceTypeFromFlag(move_type), this_col);
                RemovePiece(to);
                Zobrist::RemovePiece(state.hash, moved_piece, to);
                PutPiece(to, promoted_to);
                Zobrist::PutPiece(state.hash, promoted_to, to);
                return true;
            } else if (move_type == EN_PASSANT) {
                return !IsAttackedBySliders(Bitboards::Lsb(GetPieces(KING, this_col)), next_col,
                                            GetPieces(ALL_TYPES));
            }

            return true;
        }

        if (TypeOfPiece(moved_piece) == KING) {

            state.cr &= this_col == WHITE ? Bitboards::RankMask(RANK_8) : Bitboards::RankMask(RANK_1);
            Zobrist::UpdateCr(state.hash, previous_cr, GetCr());

            if (move_type == CASTLING) {
                Move r_move = RookCastlingMove(to, this_col);
                Square r_to = ToSquare(r_move);
                Square r_from = FromSquare(r_move);
                // put back the rook that we took out earlier in chess 960 castling
                Search::Globals::chess960 ? PutPiece(r_to, NewPiece(ROOK, this_col)) : MovePiece(r_from, r_to);
                Zobrist::MovePiece(state.hash, NewPiece(ROOK, this_col), r_from, r_to);

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

        state = history[--history_cnt];

        // ches960 castling, special case
        if (Search::Globals::chess960 && GetMoveType(m) == CASTLING) {
            Move rook_move = RookCastlingMove(to,ColorToMove());
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

    Move Board::RookCastlingMove(Square king_to, Color c) const {
        Square to = FileFromSquare(king_to) == FILE_G ? king_to - 1 : king_to + 1;
        Bitboard rook_pos = FileFromSquare(to) == FILE_F ? orig_rooks[c][SHORT] : orig_rooks[c][LONG];
        return NewMove(Bitboards::Lsb(rook_pos), to);
    }

    bool Board::MakeUciMove(const std::string &move_string) {

        Move uci_move = NewMoveFromName(move_string);
        MoveGen move_gen(*this);
        Move move;

        while ((move = move_gen.GetAnyMove())) {
            if (FromSquare(move) == FromSquare(uci_move) && ToSquare(move) == ToSquare(uci_move)) {
                if (IsPromotion(move) && move != uci_move) {
                    continue;
                }
                if(GetMoveType(uci_move) == CASTLING && GetMoveType(move) != CASTLING) {
                    continue;
                }
                MakeMove(move);
                return true;
            }
        }
        return false;
    }

    bool Board::ParseFenValidate(const std::string &fen) {

        std::istringstream iss(fen);
        std::string token;

        // board position
        iss >> token;
        File f = FILE_A;
        Rank r = RANK_8;
        bool prev_is_digit = false;

        //std::regex rgx(R"(\s*([rnbqkpRNBQKP1-8]{1,8}\/){7}([rnbqkpRNBQKP1-8]{1,8})\s[bw]\s(([a-hkqA-HKQ]{1,4})|(-))\s(([a-h][36])|(-))\s\d+\s\d+\s*)");
        //Uci::Send("Regex: " + std::to_string(std::regex_match(fen, rgx)));

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
        if (f != FILE_H + 1 || r != RANK_1) {
            return false;
        }

        // color to move
        if (iss >> token && (token == "w" || token == "b")) {
            SetColorToMove(token == "w" ? WHITE : BLACK);
        } else {
            return false;
        }

        // castling rights
        if (iss >> token) {
            if (token.length() <= 4 && Utils::ContainsOnlyChars(token, "KQkqABCDEFGHabcdefgh") && Utils::AllUniqueChars(token)) {

                for (char c : token) {
                    Color col = std::isupper(c) ? WHITE : BLACK;
                    Bitboard king = GetPieces(KING, col);
                    Bitboard rooks = GetPieces(ROOK, col);
                    rooks &= col == WHITE ? Bitboards::RankMask(RANK_1) : Bitboards::RankMask(RANK_8);
                    c = std::tolower(c, std::locale());

                    if (c == 'k') {
                        Bitboard r_square = SquareToBB(Bitboards::Msb(rooks));
                        state.cr |= r_square;
                        orig_rooks[col][SHORT] = r_square;
                    } else if (c == 'q') {
                        Bitboard r_square = SquareToBB(Bitboards::Lsb(rooks));
                        state.cr |= r_square;
                        orig_rooks[col][LONG] = r_square;
                    } else if ('a' <= c && c <= 'h') {
                        Bitboard r_square = SquareToBB(SqFromFiRa(FileFromChar(c), col == WHITE ? RANK_1 : RANK_8));
                        state.cr |= r_square;
                        r_square > king ? orig_rooks[col][SHORT] = r_square : orig_rooks[col][LONG] = r_square;
                    }
                }

                if ((state.cr & GetPieces(ROOK)) != state.cr) {
                    return false;
                }

            } else if (token != "-") {
                return false;
            }
        }

        // en passant square
        if (iss >> token) {
            if (token.length() == 2 && token[0] >= 'a' && token[0] <= 'h' && (token[1] == '3' || token[1] == '6')) {
                SetEpSquare(SqFromFiRa(FileFromChar(token[0]), RankFromChar(token[1])));
            } else if (token != "-") {
                return false;
            }
        }

        // ply
        if (iss >> token) {
            if (Utils::IsPositiveNumber(token) && std::stoi(token) <= 150) {
                SetPly(std::stoi(token));
            } else {
                return false;
            }
        }

        // move count
        if (iss >> token) {
            if (Utils::IsPositiveNumber(token) && std::stoi(token) < MAX_GAME_LENGTH && std::stoi(token) >= 1) {
                SetMoveNumber(std::stoi(token));
            } else {
                return false;
            }
        }

        return true;
    }

    std::string Board::MoveToName(Move m) const {

        if (m == ZERO_MOVE) {
            return "0000";
        }

        std::string ret = SquareToName(FromSquare(m)) + SquareToName(ToSquare(m));

        // chess 960 castling is denoted by capturing own rook
        if(Search::Globals::chess960 && GetMoveType(m) == CASTLING) {
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

    Move Board::NewMoveFromName(std::string_view move_name) const {

        Square s_from = NameToSquare(&move_name[0]);
        Square s_to = NameToSquare(&move_name[2]);

        if (move_name.length() > 4) {
            MoveType flag = move_name[4] == 'q' ? PROMOTE_QUEEN :
                            move_name[4] == 'r' ? PROMOTE_ROOK :
                            move_name[4] == 'b' ? PROMOTE_BISHOP :
                            PROMOTE_KNIGHT;
            return NewMove(s_from, s_to, flag);
        }

        if (Search::Globals::chess960) {
            Piece p = GetPieceOnSquare(s_to);
            if (ColorOfPiece(p) == ColorToMove() && TypeOfPiece(p) == ROOK) {
                if (s_to > s_from) {
                    return NewMove(s_from, ColorOfPiece(p) == WHITE ? G1 : G8, CASTLING);
                } else {
                    return NewMove(s_from, ColorOfPiece(p) == WHITE ? C1 : C8, CASTLING);
                }
            }
        }

        return NewMove(s_from, s_to);
    }

    std::string Board::PPBoard() const {

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
            << "Fullmove clock: " << TotalMoves() << " | Halfmove clock: " << Ply();

        return oss.str();
    }


}
