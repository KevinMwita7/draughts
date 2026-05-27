#pragma once
#include <cstdint>

namespace draughts {

// A playable-square index in [0, 31]; SQ_NONE used as sentinel.
using Square   = uint8_t;
// 32-bit mask: bit N is set when square N is occupied / of interest.
using Bitboard = uint32_t;

constexpr Square SQ_NONE = 32;

// ---- Color ------------------------------------------------------------------

enum Color : uint8_t { BLACK = 0, WHITE = 1 };
constexpr Color operator~(Color c) noexcept { return Color(c ^ 1); }

// ---- PieceKind --------------------------------------------------------------

enum PieceKind : uint8_t { MAN = 0, KING = 1 };

// ---- Piece ------------------------------------------------------------------
// Encoding: (color << 1) | kind  →  BLACK/MAN=0, BLACK/KING=1, WHITE/MAN=2, WHITE/KING=3
// NO_PIECE = 4 (sentinel for empty squares).

enum Piece : uint8_t {
    B_MAN    = 0,
    B_KING   = 1,
    W_MAN    = 2,
    W_KING   = 3,
    NO_PIECE = 4,
};

constexpr Piece     make_piece(Color c, PieceKind k) noexcept { return Piece((c << 1) | k); }
constexpr Color     color_of  (Piece p)              noexcept { return Color(p >> 1); }
constexpr PieceKind kind_of   (Piece p)              noexcept { return PieceKind(p & 1); }

// ---- Move -------------------------------------------------------------------

struct Move {
    Square   from;
    Square   to;
    Bitboard captured;   // bitboard of every square jumped over (all pieces removed)
    bool     promotion;  // true when a man reaches the back rank

    constexpr bool operator==(Move o) const noexcept {
        return from == o.from && to == o.to && captured == o.captured;
    }
    constexpr bool operator!=(Move o) const noexcept { return !(*this == o); }
};

constexpr Move NULL_MOVE     = { SQ_NONE, SQ_NONE, 0, false };
constexpr bool is_null      (Move m) noexcept { return m.from == SQ_NONE; }
constexpr bool is_capture   (Move m) noexcept { return m.captured != 0; }
constexpr bool is_quiet     (Move m) noexcept { return m.captured == 0; }

// ---- Score ------------------------------------------------------------------

using Score = int32_t;

constexpr Score SCORE_NONE  = -32001;
constexpr Score SCORE_DRAW  =  0;
constexpr Score SCORE_WIN   =  30000;   // forced win (side to move wins)
constexpr Score SCORE_LOSS  = -30000;   // forced loss
constexpr Score SCORE_INF   =  32000;

// ---- Limits -----------------------------------------------------------------

constexpr int MAX_MOVES = 64;   // upper bound on legal moves from any position
constexpr int MAX_PLY   = 128;  // maximum search depth

} // namespace draughts
