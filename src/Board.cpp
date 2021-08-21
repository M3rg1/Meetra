#include "Board.h"
#include "Bitboards.h"
#include <cstring>
#include <sstream>

namespace Meetra {

    Board::Board() {
        NewPosition(STARTPOS_FEN);
    }

    void Board::NewPosition(const std::string &fen) {
        history_cnt = 0;
        curr_data.state = NEW_GAME_STATE;

        std::memset(board, 0, sizeof(Piece) * SQUARE_NR);
        std::memset(color_bbs, 0, sizeof(Bitboard) * COLOR_NR);
        std::memset(type_bbs, 0, sizeof(Bitboard) * PIECE_TYPE_NR);

        ParseFen(fen);

        curr_data.hash = Zobrist::GenHash(*this);
    }

    Bitboard Board::PinnedPiecesForSquare(Square s, Color attackers_color) const {

        Bitboard pinned_pieces = EMPTY_BB;
        Bitboard potential_blockers = GetPieces(ALL_TYPES);

        Bitboard bishop_queen_attackers = GetPieces(BISHOP, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (bishop_queen_attackers) {
            Square attacker_s = Bitboards::PopLsb(bishop_queen_attackers);
            Bitboard blockers = Bitboards::GetRayBetweenSquares(attacker_s, s) & potential_blockers &
                                Bitboards::GetUnboundBishopMoves(attacker_s);
            if (blockers && !Bitboards::MoreThanOne(blockers)) {
                pinned_pieces |= blockers;
            }
        }

        Bitboard rook_queen_attackers = GetPieces(ROOK, attackers_color) | GetPieces(QUEEN, attackers_color);
        while (rook_queen_attackers) {
            Square attacker_s = Bitboards::PopLsb(rook_queen_attackers);
            Bitboard blockers = Bitboards::GetRayBetweenSquares(attacker_s, s) & potential_blockers &
                                Bitboards::GetUnboundRookMoves(attacker_s);
            if (blockers && !Bitboards::MoreThanOne(blockers)) {
                pinned_pieces |= blockers;
            }
        }

        return pinned_pieces;
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

    void Board::RemovePiece(Square s, ZobristHash &h) {
        Piece p = board[s];
        RemovePiece(s);
        Zobrist::RemovePiece(h, TypeOfPiece(p), ColorOfPiece(p), s);
    }

    void Board::PutPiece(Square s, Piece p, ZobristHash &h) {
        PutPiece(s, p);
        Zobrist::AddPiece(h, TypeOfPiece(p), ColorOfPiece(p), s);
    }

    void Board::MovePiece(Square from, Square to, ZobristHash &h) {
        Piece p = board[from];
        MovePiece(from, to);
        Zobrist::MovePiece(h, TypeOfPiece(p), ColorOfPiece(p), from, to);
    }

    bool Board::MakeMove(Move m) {

        history[history_cnt++] = curr_data;

        Color this_move_col = ColorToMove();
        ChangeColorToMove();
        Color next_move_col = ColorToMove();
        Zobrist::UpdateColor(curr_data.hash, next_move_col);
        ClearCapturedPiece();
        IncrementPly();
        IncrementMoveNumber(this_move_col);

        if (EpSquare()) {
            Zobrist::RemoveEp(curr_data.hash, EpSquare());
            ClearEpSquare();
        }

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        CastlingRights previous_cr = GetCR();
        RemoveCastlingRights(static_cast<CastlingRights>(castling_mask[from] | castling_mask[to]));
        Zobrist::UpdateCr(curr_data.hash, previous_cr, GetCR());

        MoveType move_type = GetMoveType(m);
        Piece captured_piece = board[to];
        PieceType moved_piece_type = TypeOfPiece(board[from]);

        if (captured_piece || move_type == EN_PASSANT) {
            Square capture_square = to;
            if (move_type == EN_PASSANT) {
                capture_square += next_move_col ? SOUTH : NORTH;
                captured_piece = NewPiece(PAWN, next_move_col);
            }
            RemovePiece(capture_square, curr_data.hash);
            SetCapturedPiece(captured_piece);
            ResetPly();
        } else if (moved_piece_type == PAWN) {
            ResetPly();
        }

        MovePiece(from, to, curr_data.hash);

        if (move_type) {
            if (move_type == TWO_FORWARD) {
                SetEpSquare(next_move_col ? to + SOUTH : to + NORTH);
                Zobrist::AddEp(curr_data.hash, EpSquare());
            } else if (move_type == CASTLING) {
                MovePiece(RookFromCastling(to), RookToCastling(to), curr_data.hash);
                return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                               GetPieces(ALL_TYPES));
            } else if (IsPromotion(m)) {
                RemovePiece(to, curr_data.hash);
                PutPiece(to, NewPiece(PieceTypeFromFlag(move_type), this_move_col), curr_data.hash);
            } else if (move_type == EN_PASSANT) {
                return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                               GetPieces(ALL_TYPES));
            }
        } else if (moved_piece_type == KING) {
            return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                           GetPieces(ALL_TYPES));
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
            if (GetMoveType(m) == EN_PASSANT) {
                to = ColorToMove() ? to + NORTH : to + SOUTH;
            }
            PutPiece(to, captured_piece);
        }

        if (GetMoveType(m) == CASTLING) {
            MovePiece(RookToCastling(to), RookFromCastling(to));
            PutPiece(from, NewPiece(KING, ColorToMove()));
        } else if (IsPromotion(m)) {
            RemovePiece(from);
            PutPiece(from, NewPiece(PAWN, ColorToMove()));
        }
    }

    Piece CharToPiece(char c) {
        if (c == 'P') return W_PAWN;
        else if (c == 'N') return W_KNIGHT;
        else if (c == 'B') return W_BISHOP;
        else if (c == 'R') return W_ROOK;
        else if (c == 'Q') return W_QUEEN;
        else if (c == 'K') return W_KING;
        else if (c == 'p') return B_PAWN;
        else if (c == 'n') return B_KNIGHT;
        else if (c == 'b') return B_BISHOP;
        else if (c == 'r') return B_ROOK;
        else if (c == 'q') return B_QUEEN;
        else if (c == 'k') return B_KING;
        else return NO_PIECE;
    }

    char PieceToChar(Piece p) {
        if (p == W_PAWN) return 'P';
        else if (p == W_KNIGHT) return 'N';
        else if (p == W_BISHOP) return 'B';
        else if (p == W_ROOK) return 'R';
        else if (p == W_QUEEN) return 'Q';
        else if (p == W_KING) return 'K';
        else if (p == B_PAWN) return 'p';
        else if (p == B_KNIGHT) return 'n';
        else if (p == B_BISHOP) return 'b';
        else if (p == B_ROOK) return 'r';
        else if (p == B_QUEEN) return 'q';
        else if (p == B_KING) return 'k';
        else return 'o';
    }

    void Board::ParseFen(const std::string &fen) {

        std::istringstream iss(fen);
        std::string token;

        // board position
        iss >> token;
        File f = FILE_A;
        Rank r = RANK_8;
        for (char c : token) {
            if (c == '/') {
                f = FILE_A;
                --r;
            } else if (std::isdigit(c)) {
                int empty_squares = c - '0';
                f += empty_squares;
            } else {
                PutPiece(SquareFromFiRa(f, r), CharToPiece(c));
                ++f;
            }
        }

        // color to move
        if (iss >> token) {
            SetColorToMove(token == "w" ? WHITE : BLACK);
        }

        // castling rights
        if (iss >> token) {
            if (token.find('K') != std::string::npos) SetCastlingRights(WHITE_SHORT);
            if (token.find('Q') != std::string::npos) SetCastlingRights(WHITE_LONG);
            if (token.find('k') != std::string::npos) SetCastlingRights(BLACK_SHORT);
            if (token.find('q') != std::string::npos) SetCastlingRights(BLACK_LONG);
        }

        // en passant square
        if (iss >> token) {
            if (token != "-") {
                File file = FileFromChar(token[0]);
                Rank rank = RankFromChar(token[1]);
                SetEpSquare(SquareFromFiRa(file, rank));
            }
        }

        // ply
        if (iss >> token) {
            if (token != "-") {
                SetPly(std::stoi(token));
            }
        }

        // move count
        if (iss >> token) {
            if (token != "-") {
                SetMoveNumber(std::stoi(token));
            }
        }
    }

    std::string Board::PPBoard() const {

        std::ostringstream oss;

        for (Rank r = RANK_8; r >= RANK_1; --r) {
            oss << (r + 1) << " |";
            for (File f = FILE_A; f <= FILE_H; ++f) {
                oss << ' ' << PieceToChar(board[SquareFromFiRa(f, r)]) << ' ';
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
        oss << " | EP square: " << (EpSquare() == SQUARE_ZERO ? "-" : std::to_string(EpSquare())) << '\n'
            << "Fullmove clock: " << TotalMoves() << " | Halfmove clock: " << Ply();

        return oss.str();
    }


}
