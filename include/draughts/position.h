#pragma once
#include "bitboard.h"

namespace draughts {

    // Position: complete game state, no history (history lives on the call stack).
    struct Position {
        // bb[color][kind]: one bitboard per (Color, PieceKind) combination.
        Bitboard bb[2][2];

        // Derived (recomputed by rebuild_derived after any mutation).
        Bitboard occupied;
        Bitboard empty_sq;

        Color    side_to_move;
        uint64_t hash;           // Zobrist hash
        uint16_t ply;            // half-moves from the root (look-ahead distance)
        uint8_t  reversible;     // half-moves without a capture (draw-rule counter)

        // Queries

        Bitboard pieces(Color c)              const noexcept { return bb[c][MAN] | bb[c][KING]; }
        Bitboard pieces(Color c, PieceKind k) const noexcept { return bb[c][k]; }
        Bitboard men   (Color c)              const noexcept { return bb[c][MAN]; }
        Bitboard kings (Color c)              const noexcept { return bb[c][KING]; }

        int      count (Color c)              const noexcept { return popcount(pieces(c)); }
        int      count (Color c, PieceKind k) const noexcept { return popcount(bb[c][k]); }
        int      total ()                     const noexcept { return popcount(occupied); }

        Piece    piece_on(Square s) const noexcept;

        bool     is_terminal() const noexcept;  // side_to_move has no legal move

        // Mutation

        void do_move  (Move& m);
        void undo_move(Move m);  // m must be the move passed to the matching do_move

        // Recompute occupied / empty_sq from bb[][].
        void rebuild_derived() noexcept;

        // Factory

        static Position start_position();
        static Position empty_position();
    };
}
