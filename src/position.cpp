#include "draughts/position.h"
#include "draughts/bitboard.h"
#include "draughts/zobrist.h"

namespace draughts {

    void Position::do_move(Move m) {
        const Color   us   = side_to_move;
        const Color   them = ~us;
        const PieceKind kind = (bb[us][KING] & sq_bb(m.from)) ? KING : MAN;

        // Update hash before mutating bitboards (captured piece types still readable)
        hash ^= zobrist::piece_key(m.from, make_piece(us, kind));
        Bitboard caps = m.captured;
        while (caps) {
            Square s = pop_lsb(caps);
            PieceKind ck = (bb[them][KING] & sq_bb(s)) ? KING : MAN;
            hash ^= zobrist::piece_key(s, make_piece(them, ck));
        }
        hash ^= zobrist::piece_key(m.to, make_piece(us, m.promotion ? KING : kind));
        hash ^= zobrist::SIDE_KEY;

        // Mutate bitboards
        bb[us][kind]   ^= sq_bb(m.from) | sq_bb(m.to);
        bb[them][MAN]  &= ~m.captured;
        bb[them][KING] &= ~m.captured;
        if (m.promotion) {
            bb[us][MAN]  &= ~sq_bb(m.to);
            bb[us][KING] |=  sq_bb(m.to);
        }

        occupied = bb[BLACK][MAN] | bb[BLACK][KING] | bb[WHITE][MAN] | bb[WHITE][KING];
        empty_sq = ~occupied;

        side_to_move = them;
        ++ply;
        reversible = m.captured ? 0 : reversible + 1;
    }

    Piece Position::piece_on(Square s) const noexcept {
        Bitboard mask = sq_bb(s);
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < 2; ++k)
                if (bb[c][k] & mask) return make_piece(Color(c), PieceKind(k));
        return NO_PIECE;
    }

} // namespace draughts
