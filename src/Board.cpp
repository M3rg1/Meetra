#include "Board.h"
#include "Bitboards.h"
#include "Misc.h"
#include <cstring>
#include "ZobristHash.h"
#include "StringTokenStream.h"
#include <sstream>

namespace Meetra {

    // TODO move the initialization from fen to SetPosition function, and make it more efficient (directly read into the
    // bitboards, arrays, game state and such so we dont have to copy everything, this takes forever
    // make LoadFen function that takes in game state and other arrays as arguments and fills them

    Board::Board() {
        NewPosition(STARTPOS_FEN);
    }

    void Board::NewPosition(const std::string &fen) {
        history_cnt = 0;
        current_state.game_state = NEW_GAME_STATE;

        std::memset(board, 0, sizeof(*board) * SQUARE_NR);
        std::memset(color_bbs, 0, sizeof(*color_bbs) * COLOR_NR);
        std::memset(type_bbs, 0, sizeof(*type_bbs) * PIECE_TYPE_NR);

        ParseFen(fen);

        current_state.zobrist_hash = Zobrist::GenHash(*this);
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

    bool Board::MakeMove(Move m) {

        board_history[history_cnt++] = current_state;

        Color this_move_col = ColorToMove();
        ChangeColorToMove();
        Color next_move_col = ColorToMove();

        ClearCapturedPiece();
        ClearEpSquare();
        IncrementPly();

        IncrementMoveNumber(this_move_col);

        Square from = FromSquare(m);
        Square to = ToSquare(m);

        RemoveCastlingRights(static_cast<CastlingRights>(castling_mask[from] | castling_mask[to]));

        MoveType move_type = GetMoveType(m);
        Piece captured_piece = board[to];
        PieceType moved_piece_type = TypeOfPiece(board[from]);


        if (captured_piece || move_type == EN_PASSANT) {
            Square capture_square = to;
            if (move_type == EN_PASSANT) {
                capture_square += next_move_col ? SOUTH : NORTH;
                captured_piece = NewPiece(PAWN, next_move_col);
            }
            RemovePiece(capture_square);
            SetCapturedPiece(captured_piece);
            ResetPly();
        } else if (moved_piece_type == PAWN) {
            ResetPly();
        }

        MovePiece(from, to);

        if (move_type) {
            if (move_type == TWO_FORWARD) {
                SetEpSquare(next_move_col ? to + SOUTH : to + NORTH);
            } else if (move_type == CASTLING) {
                MovePiece(RookFromCastling(to), RookToCastling(to));
                current_state.zobrist_hash = Zobrist::GenHash(*this);
                return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                         GetPieces(ALL_TYPES));
            } else if (IsPromotion(m)) {
                RemovePiece(to);
                PutPiece(to, NewPiece(PieceTypeFromFlag(move_type), this_move_col));
            } else if (move_type == EN_PASSANT) {
                current_state.zobrist_hash = Zobrist::GenHash(*this);
                return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                         GetPieces(ALL_TYPES));
            }
        } else if (moved_piece_type == KING) {
            current_state.zobrist_hash = Zobrist::GenHash(*this);
            return !IsSquareAttacked(Bitboards::Lsb(GetPieces(KING, this_move_col)), next_move_col,
                                     GetPieces(ALL_TYPES));
        }

        current_state.zobrist_hash = Zobrist::GenHash(*this);
        return true;
    }

    void Board::UnmakeMove(Move m) {

        Square from = FromSquare(m);
        Square to = ToSquare(m);
        Piece captured_piece = CapturedPiece();

        MovePiece(to, from);

        if (captured_piece) {
            if (GetMoveType(m) == EN_PASSANT) {
                to = ColorToMove() ? to + SOUTH : to + NORTH;
            }
            PutPiece(to, captured_piece);
        }

        if (GetMoveType(m) == CASTLING) {
            MovePiece(RookToCastling(to), RookFromCastling(to));
            PutPiece(from, NewPiece(KING, OtherColor(ColorToMove())));
        } else if (IsPromotion(m)) {
            RemovePiece(from);
            PutPiece(from, NewPiece(PAWN, OtherColor(ColorToMove())));
        }

        current_state = board_history[--history_cnt];
    }

    void Board::ParseFen(const std::string &fen) {

        StringTokenStream sts(fen);

        std::string board_pos_fen = sts.NextToken();
        File f = FILE_A;
        Rank r = RANK_8;
        for (char c : board_pos_fen) {
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

        if (sts.HasNext()) {
            SetColorToMove(sts.NextToken() == "w" ? WHITE : BLACK);
        }

        if (sts.HasNext()) {
            std::string castling_rights = sts.NextToken();
            if (castling_rights.find('K') != std::string::npos) SetCastlingRights(WHITE_SHORT);
            if (castling_rights.find('Q') != std::string::npos) SetCastlingRights(WHITE_LONG);
            if (castling_rights.find('k') != std::string::npos) SetCastlingRights(BLACK_SHORT);
            if (castling_rights.find('q') != std::string::npos) SetCastlingRights(BLACK_LONG);
        }

        if (sts.HasNext()) {
            std::string ep_info = sts.NextToken();
            if (ep_info != "-") {
                File file = FileFromChar(ep_info[0]);
                Rank rank = RankFromChar(ep_info[1]);
                SetEpSquare(SquareFromFiRa(file, rank));
            }
        }

        if (sts.HasNext()) {
            std::string ply = sts.NextToken();
            if (ply != "-") {
                SetPly(std::stoi(ply));
            }
        }

        if (sts.HasNext()) {
            std::string move_count = sts.NextToken();
            if (move_count != "-") {
                SetMoveNumber(std::stoi(move_count));
            }
        }
    }

    std::string Board::PPBoard() const {

        std::stringstream ss;

        for (Rank r = RANK_8; r >= RANK_1; --r) {
            ss << std::to_string(r + 1) << " |";
            for (File f = FILE_A; f <= FILE_H; ++f) {
                ss << ' ' << PieceToChar(board[SquareFromFiRa(f, r)]) << ' ';
            }
            ss << '\n';
        }
        ss << "---------------------------\n"
           << "  | A  B  C  D  E  F  G  H\n\n"
           << "Player to move: " << (ColorToMove() == WHITE ? "white\n" : "black\n")
           << "Castling rights: ";
        if (!CanCastleAny()) {
            ss << '-';
        } else {
            if (CanWhiteShortCR()) ss << 'K';
            if (CanWhiteLongCR()) ss << 'Q';
            if (CanBlackShortCR()) ss << 'k';
            if (CanBlackLongCR()) ss << 'q';
        }
        ss << " | EP square: " << (EpSquare() == SQUARE_ZERO ? "-" : std::to_string(EpSquare())) << '\n'
           << "Fullmove clock: " << TotalMoves() << " | Halfmove clock: " << Ply();

        return ss.str();
    }


}
