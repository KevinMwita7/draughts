#pragma once
#include "position.h"

namespace draughts {

    // Fixed-size move list; sized to MAX_MOVES (no heap allocation).
    struct MoveList {
        Move moves[MAX_MOVES];
        int  count = 0;

        void push(Move m)           noexcept { moves[count++] = m; }
        void clear()                noexcept { count = 0; }
        bool empty()          const noexcept { return count == 0; }

        Move*       begin()         noexcept { return moves; }
        Move*       end()           noexcept { return moves + count; }
        const Move* begin()   const noexcept { return moves; }
        const Move* end()     const noexcept { return moves + count; }
        Move&       operator[](int i)       noexcept { return moves[i]; }
        const Move& operator[](int i) const noexcept { return moves[i]; }
    };

    // Generate all legal moves for pos.side_to_move.
    // Captures are mandatory: if any capture exists only captures are returned.
    void generate_moves   (const Position& pos, MoveList& list);

    // Generate only capture moves (used in quiescence search).
    void generate_captures(const Position& pos, MoveList& list);

    // Generate only quiet (non-capture) moves.
    void generate_quiets  (const Position& pos, MoveList& list);

    // True if side_to_move has at least one legal move (avoids full generation).
    bool has_legal_moves  (const Position& pos);

    // True if side_to_move has at least one capture available.
    bool has_captures     (const Position& pos);

}
