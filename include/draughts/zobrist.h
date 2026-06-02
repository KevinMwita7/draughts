#pragma once
#include "types.h"

namespace draughts {

    // Zobrist keys: one random 64-bit value per (square, piece) pair plus one for
    // side-to-move. XOR all keys for occupied squares to get the position hash;
    // XOR the side key when it is WHITE's turn.
    //
    // Usage in do_move / undo_move:
    //   hash ^= piece_key(s, p);   // remove piece from square
    //   hash ^= piece_key(t, p);   // place piece on square
    //   hash ^= SIDE_KEY;          // flip side to move
    //
    // Keys are initialised once at program start by init_zobrist().

    namespace zobrist {

        extern uint64_t PIECE_KEY[32][4];  // [square][Piece enum value 0-3]
        extern uint64_t SIDE_KEY;          // XORed in when side_to_move == WHITE

        void init();   // call once at startup (fills keys with pseudo-random values)

        inline uint64_t piece_key(Square s, Piece p) noexcept { return PIECE_KEY[s][p]; }

    } // namespace zobrist

    // Compute the full Zobrist hash for pos from scratch.
    // Called by start_position(), parse_pdn_position(), and after any bulk mutation.
    uint64_t compute_hash(const struct Position& pos) noexcept;

}
