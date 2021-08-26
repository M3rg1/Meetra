#include "Board.h"
#include "Bitboards.h"
#include <sstream>
#include "Uci.h"
#include "Utils.h"
#include "MoveGen.h"

namespace Meetra {

    Board::Board() {
        NewPosition(STARTPOS_FEN);
    }

    bool Board::NewPosition(const std::string &fen) {

        Board previous = *this;

        history_cnt = 0;
        curr_data.state = NEW_GAME_STATE;

        std::fill(std::begin(board), std::end(board), NO_PIECE);
        std::fill(std::begin(color_bbs), std::end(color_bbs), EMPTY_BB);
        std::fill(std::begin(type_bbs), std::end(type_bbs), EMPTY_BB);

        if (!ParseFenValidate(fen) || !IsBoardValid()) {
            *this = previous;
            return false;
        }

        curr_data.hash = Zobrist::GenHash(*this);

        return true;
    }

    bool Board::IsBoardValid() const {

        Bitboard white_king = GetPieces(KING, WHITE);
        if (Bitboards::ExactlyOne(white_king) != 1) {
            return false;
        }

        Bitboard black_king = GetPieces(KING, BLACK);
        if (Bitboards::ExactlyOne(black_king) != 1) {
            return false;
        }

        Color now_move = ColorToMove();
        Square enemy_king_square = now_move == WHITE ? Bitboards::Lsb(black_king) : Bitboards::Lsb(white_king);
        if (IsSquareAttacked(enemy_king_square, now_move, GetPieces(ALL_TYPES))) {
            return false;
        }

        return true;
    }

    Bitboard Board::PinnedPiecesForSquare(Square s, Color attackers_color) const {

        Bitboard pinned_pieces = EMPTY_BB;
        Bitboard potential_blockers = GetPieces(ALL_TYPES);

        Bitboard bishop_queen_attackers = GetPieces(BISHOP, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (bishop_queen_attackers) {
            Square attacker_s = Bitboards::PopLsb(bishop_queen_attackers);
            Bitboard blockers = Bitboards::GetRayBetweenSquares(attacker_s, s) & potential_blockers &
                                Bitboards::GetUnboundBishopMoves(attacker_s);
            if (Bitboards::ExactlyOne(blockers)) {
                pinned_pieces |= blockers;
            }
        }

        Bitboard rook_queen_attackers = GetPieces(ROOK, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (rook_queen_attackers) {
            Square attacker_s = Bitboards::PopLsb(rook_queen_attackers);
            Bitboard blockers = Bitboards::GetRayBetweenSquares(attacker_s, s) & potential_blockers &
                                Bitboards::GetUnboundRookMoves(attacker_s);
            if (Bitboards::ExactlyOne(blockers)) {
                pinned_pieces |= blockers;
            }
        }

        return pinned_pieces;
    }

    // takes a pseudo legal move and checks whether it is legal
    bool Board::IsMoveLegal(Move m) {

        if (TypeOfPiece(board[FromSquare(m)]) == KING) {
            Color enemy_color = OtherColor(ColorToMove());
            Bitboard occ = GetPieces(ALL_TYPES);
            if (GetMoveType(m) == CASTLING) {
                Square to = ToSquare(m);
                return !IsSquareAttacked(to, enemy_color, occ) &&
                       !IsSquareAttacked(RookMoveTo(FromSquare(m), to), enemy_color, occ);
            }
            occ ^= SquareToBB(FromSquare(m));
            return !IsSquareAttacked(ToSquare(m), enemy_color, occ);
        }

        if (GetMoveType(m) == EN_PASSANT) {
            Color my_color = ColorToMove();
            Color enemy_color = OtherColor(my_color);
            Square from = FromSquare(m);
            Square to = ToSquare(m);
            Square take_square = my_color == WHITE ? to + SOUTH : to + NORTH;

            MovePiece(from, to);
            RemovePiece(take_square);

            bool ok = !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, my_color)), enemy_color, GetPieces(ALL_TYPES));

            MovePiece(to, from);
            PutPiece(take_square, NewPiece(PAWN, enemy_color));

            return ok;
        }

        return true;
    }

    bool Board::IsSquareAttacked(Square s, Color attacked_by, Bitboard occ) const {
        return Bitboards::GetAttacksForPiece<ROOK>(s, occ) &
               (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
               Bitboards::GetAttacksForPiece<BISHOP>(s, occ) &
               (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by)) ||
               Bitboards::GetAttacksForPiece<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by) ||
               Bitboards::GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by) ||
               Bitboards::GetAttacksForPiece<KING>(s) & GetPieces(KING, attacked_by);
    }

    Bitboard Board::SquareAttackers(Square s, Color attacked_by, Bitboard occ) const {
        return (Bitboards::GetAttacksForPiece<PAWN>(s, occ, OtherColor(attacked_by)) & GetPieces(PAWN, attacked_by)) |
               (Bitboards::GetAttacksForPiece<KNIGHT>(s) & GetPieces(KNIGHT, attacked_by)) |
               (Bitboards::GetAttacksForPiece<BISHOP>(s, occ) &
                (GetPieces(BISHOP, attacked_by) | GetPieces(QUEEN, attacked_by))) |
               (Bitboards::GetAttacksForPiece<ROOK>(s, occ) &
                (GetPieces(ROOK, attacked_by) | GetPieces(QUEEN, attacked_by))) |
               (Bitboards::GetAttacksForPiece<KING>(s) & GetPieces(KING, attacked_by));
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
        board[to] = p;
        board[from] = NO_PIECE;
        Bitboard from_to = SquareToBB(from) | SquareToBB(to);
        color_bbs[ColorOfPiece(p)] ^= from_to;
        type_bbs[TypeOfPiece(p)] ^= from_to;
        type_bbs[ALL_TYPES] ^= from_to;
    }

    // Make a pseudo legal move.
    // If the move turns out to be illegal, it has to be undone via the UnmakeMove function.
    // It is better to assume all attempted moves are legal, rather than first verifying the legality and only then
    // making them. It is so, because only a very few moves generated by our move gen actually turn out to be illegal,
    // and unmaking a move is a very cheap operation in any case.
    bool Board::MakeMove(Move m) {

        history[history_cnt++] = curr_data;

        Color this_move_col = ColorToMove();
        ChangeColorToMove();
        Color next_move_col = ColorToMove();
        Zobrist::UpdateColor(curr_data.hash, next_move_col);

        IncrementMoveNumber(this_move_col);
        IncrementPly();
        ClearCapturedPiece();
        if (EpSquare()) {
            Zobrist::RemoveEp(curr_data.hash, EpSquare());
            ClearEpSquare();
        }

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        if (GetCR()) {
            CastlingRights previous_cr = GetCR();
            RemoveCastlingRights(static_cast<CastlingRights>(castling_mask[from] | castling_mask[to]));
            if (previous_cr != GetCR()) {
                Zobrist::UpdateCr(curr_data.hash, previous_cr, GetCR());
            }
        }

        MoveType move_type = GetMoveType(m);
        Piece captured_piece = move_type == EN_PASSANT ? NewPiece(PAWN, next_move_col) : GetPieceOnSquare(to);

        if (captured_piece) {
            Square capture_square = move_type == EN_PASSANT ? (this_move_col == WHITE ? to + SOUTH : to + NORTH) : to;
            RemovePiece(capture_square);
            Zobrist::RemovePiece(curr_data.hash, captured_piece, capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        }

        Piece moved_piece = GetPieceOnSquare(from);
        MovePiece(from, to);
        Zobrist::MovePiece(curr_data.hash, moved_piece, from, to);

        if (TypeOfPiece(moved_piece) == PAWN) {
            ResetPly();
            if (move_type == TWO_FORWARD) {
                SetEpSquare(this_move_col == WHITE ? to + SOUTH : to + NORTH);
                Zobrist::AddEp(curr_data.hash, EpSquare());
                return true;
            } else if (IsPromotion(m)) {
                Piece promoted_to = NewPiece(PieceTypeFromFlag(move_type), this_move_col);
                RemovePiece(to);
                Zobrist::RemovePiece(curr_data.hash, moved_piece, to);
                PutPiece(to, promoted_to);
                Zobrist::PutPiece(curr_data.hash, promoted_to, to);
                return true;
            } else if (move_type == EN_PASSANT) {
                return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                         GetPieces(ALL_TYPES));
            }
            return true;
        } else if (TypeOfPiece(moved_piece) == KING) {
            if (move_type == CASTLING) {
                Square rook_to = RookMoveTo(from, to);
                Square rook_from = RookMoveFrom(from, to);
                MovePiece(rook_from, rook_to);
                Zobrist::MovePiece(curr_data.hash, NewPiece(ROOK, this_move_col), rook_from, rook_to);
                return !IsSquareAttacked(rook_to, next_move_col, GetPieces(ALL_TYPES)) &&
                       !IsSquareAttacked(to, next_move_col, GetPieces(ALL_TYPES));
            }
            return !IsSquareAttacked(to, next_move_col, GetPieces(ALL_TYPES));
        }

        return true;
    }

    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Piece captured_piece = CapturedPiece();

        curr_data = history[--history_cnt];

        MovePiece(to, from);

        if (captured_piece) {
            PutPiece(GetMoveType(m) == EN_PASSANT ? ColorToMove() == WHITE ? to + SOUTH : to + NORTH : to,
                     captured_piece);
        }

        if (IsPromotion(m)) {
            RemovePiece(from);
            PutPiece(from, NewPiece(PAWN, ColorToMove()));
        } else if (GetMoveType(m) == CASTLING) {
            MovePiece(RookMoveTo(from, to), RookMoveFrom(from, to));
            PutPiece(from, NewPiece(KING, ColorToMove()));
        }
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

    Piece CharToPiece(char c) {
        // if not found -> std::string:npos + 1 == 0 == NO_PIECE, else -> index + 1 == desired piece
        static const std::string pieces = "PNBRQK  pnbrqk";
        return static_cast<Piece>(pieces.find(c) + 1);
    }

    char PieceToChar(Piece p) {
        static constexpr char pieces[] = "oPNBRQK  pnbrqk";
        return pieces[p];
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
        if (token.find('K') != std::string::npos) SetCastlingRights(WHITE_SHORT);
        if (token.find('Q') != std::string::npos) SetCastlingRights(WHITE_LONG);
        if (token.find('k') != std::string::npos) SetCastlingRights(BLACK_SHORT);
        if (token.find('q') != std::string::npos) SetCastlingRights(BLACK_LONG);

        iss >> token;
        if (token != "-") {
            File file = FileFromChar(token[0]);
            Rank rank = RankFromChar(token[1]);
            SetEpSquare(SquareFromFiRa(file, rank));
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
                PutPiece(SquareFromFiRa(f, r), CharToPiece(c));
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
            if (Utils::ContainsOnlyChars(token, "KQkq") && Utils::AllUniqueChars(token)) {
                if (token.find('K') != std::string::npos) SetCastlingRights(WHITE_SHORT);
                if (token.find('Q') != std::string::npos) SetCastlingRights(WHITE_LONG);
                if (token.find('k') != std::string::npos) SetCastlingRights(BLACK_SHORT);
                if (token.find('q') != std::string::npos) SetCastlingRights(BLACK_LONG);
            } else if (token != "-") {
                return false;
            }
        }

        // en passant square
        if (iss >> token) {
            if (token.length() == 2 && token[0] >= 'a' && token[0] <= 'h' && token[1] >= '1' && token[1] <= '8') {
                File file = FileFromChar(token[0]);
                Rank rank = RankFromChar(token[1]);
                SetEpSquare(SquareFromFiRa(file, rank));
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

    std::string Board::PPBoard() const {

        std::ostringstream oss;

        for (Rank r = RANK_8; r >= RANK_1; --r) {
            oss << std::to_string(r + 1) << " |";
            for (File f = FILE_A; f <= FILE_H; ++f) {
                oss << ' ' << PieceToChar(GetPieceOnSquare(SquareFromFiRa(f, r))) << ' ';
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
            if (CanWhiteShortCR()) oss << 'K';
            if (CanWhiteLongCR()) oss << 'Q';
            if (CanBlackShortCR()) oss << 'k';
            if (CanBlackLongCR()) oss << 'q';
        }
        oss << " | EP square: " << (EpSquare() == SQUARE_ZERO ? "-" : SquareToName(EpSquare())) << '\n'
            << "Fullmove clock: " << TotalMoves() << " | Halfmove clock: " << Ply();

        return oss.str();
    }


}
