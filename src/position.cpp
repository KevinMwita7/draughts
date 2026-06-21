#include "draughts/position.h"

#include "draughts/bitboard.h"
#include "draughts/movegen.h"
#include "draughts/zobrist.h"

namespace draughts {

void Position::do_move(Move& m) {
  const Color us = side_to_move;
  const Color them = ~us;
  const PieceKind kind = (bb[us][KING] & sq_bb(m.from)) ? KING : MAN;

  m.prev_reversible = reversible;
  m.captured_kings = bb[them][KING] & m.captured;

  hash ^= zobrist::piece_key(m.from, make_piece(us, kind));
  Bitboard caps = m.captured;
  while (caps) {
    Square s = pop_lsb(caps);
    PieceKind ck = (m.captured_kings & sq_bb(s)) ? KING : MAN;
    hash ^= zobrist::piece_key(s, make_piece(them, ck));
  }
  hash ^= zobrist::piece_key(m.to, make_piece(us, m.promotion ? KING : kind));
  hash ^= zobrist::SIDE_KEY;

  // Mutate bitboards
  bb[us][kind] ^= sq_bb(m.from) | sq_bb(m.to);
  bb[them][MAN] &= ~m.captured;
  bb[them][KING] &= ~m.captured;
  if (m.promotion) {
    bb[us][MAN] &= ~sq_bb(m.to);
    bb[us][KING] |= sq_bb(m.to);
  }

  occupied =
      bb[BLACK][MAN] | bb[BLACK][KING] | bb[WHITE][MAN] | bb[WHITE][KING];
  empty_sq = ~occupied;

  side_to_move = them;
  ++ply;
  reversible = m.captured ? 0 : reversible + 1;
}

void Position::undo_move(Move m) {
  const Color us = ~side_to_move;
  const Color them = side_to_move;
  const PieceKind kind = m.promotion                    ? KING
                         : (bb[us][KING] & sq_bb(m.to)) ? KING
                                                        : MAN;
  const PieceKind orig = m.promotion ? MAN : kind;

  hash ^= zobrist::piece_key(m.to, make_piece(us, kind));
  Bitboard caps = m.captured;
  while (caps) {
    Square s = pop_lsb(caps);
    PieceKind ck = (m.captured_kings & sq_bb(s)) ? KING : MAN;
    hash ^= zobrist::piece_key(s, make_piece(them, ck));
  }
  hash ^= zobrist::piece_key(m.from, make_piece(us, orig));
  hash ^= zobrist::SIDE_KEY;

  if (m.promotion) {
    bb[us][KING] &= ~sq_bb(m.to);
    bb[us][MAN] |= sq_bb(m.to);
  }
  bb[us][orig] ^= sq_bb(m.to) | sq_bb(m.from);
  bb[them][KING] |= m.captured_kings;
  bb[them][MAN] |= m.captured & ~m.captured_kings;

  occupied =
      bb[BLACK][MAN] | bb[BLACK][KING] | bb[WHITE][MAN] | bb[WHITE][KING];
  empty_sq = ~occupied;

  side_to_move = us;
  --ply;
  reversible = m.prev_reversible;
}

void Position::rebuild_derived() noexcept {
  occupied =
      bb[BLACK][MAN] | bb[BLACK][KING] | bb[WHITE][MAN] | bb[WHITE][KING];
  empty_sq = ~occupied;
}

Piece Position::piece_on(Square s) const noexcept {
  Bitboard mask = sq_bb(s);
  for (int c = 0; c < 2; ++c)
    for (int k = 0; k < 2; ++k)
      if (bb[c][k] & mask) return make_piece(Color(c), PieceKind(k));
  return NO_PIECE;
}

Position Position::start_position() {
  Position pos{};
  pos.bb[BLACK][MAN] = 0x00000FFF;
  pos.bb[BLACK][KING] = 0;
  pos.bb[WHITE][MAN] = 0xFFF00000;
  pos.bb[WHITE][KING] = 0;
  pos.occupied = pos.bb[BLACK][MAN] | pos.bb[BLACK][KING] | pos.bb[WHITE][MAN] |
                 pos.bb[WHITE][KING];
  pos.empty_sq = ~pos.occupied;
  pos.side_to_move = BLACK;
  pos.ply = 0;
  pos.reversible = 0;
  pos.hash = compute_hash(pos);
  return pos;
}

Position Position::empty_position() {
  Position pos{};
  pos.empty_sq = ALL_SQUARES;
  return pos;
}

bool Position::is_terminal() const noexcept { return !has_legal_moves(*this); }

}  // namespace draughts
