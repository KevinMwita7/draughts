#include "draughts/position.h"
#include "draughts/bitboard.h"

namespace draughts {

Piece Position::piece_on(Square s) const noexcept {
    Bitboard mask = sq_bb(s);
    for (int c = 0; c < 2; ++c)
        for (int k = 0; k < 2; ++k)
            if (bb[c][k] & mask) return make_piece(Color(c), PieceKind(k));
    return NO_PIECE;
}

} // namespace draughts
