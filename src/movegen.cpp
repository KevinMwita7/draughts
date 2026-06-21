#include "draughts/movegen.h"

#include "draughts/bitboard.h"
#include "draughts/position.h"

using namespace draughts;

// capture chain recursion
static void capture_from(Square orig, Square cur, Bitboard enemy,
                         Bitboard empty_bb, Bitboard captured, bool is_king,
                         Color us, MoveList& list) {
  const Bitboard promo_rank =
      (us == BLACK) ? BLACK_PROMO_RANK : WHITE_PROMO_RANK;
  const Bitboard c = sq_bb(cur);

  // Man reaching the back rank: crown it and stop — no king continuation this
  // turn
  if (!is_king && (c & promo_rank)) {
    list.push({orig, cur, captured, true});
    return;
  }

  bool any = false;

  auto try_jump = [&](Bitboard mid_bb, Bitboard to_bb) {
    if (!(mid_bb & enemy)) return;
    if (!(to_bb & empty_bb)) return;
    any = true;
    capture_from(orig, lsb(to_bb), enemy & ~mid_bb,
                 (empty_bb | c | mid_bb) & ~to_bb, captured | mid_bb, is_king,
                 us, list);
  };

  if (is_king || us == BLACK) {
    Bitboard mid, to;
    mid = shift_up_right(c);
    to = shift_up_right(mid);
    try_jump(mid, to);
    mid = shift_up_left(c);
    to = shift_up_left(mid);
    try_jump(mid, to);
  }
  if (is_king || us == WHITE) {
    Bitboard mid, to;
    mid = shift_down_right(c);
    to = shift_down_right(mid);
    try_jump(mid, to);
    mid = shift_down_left(c);
    to = shift_down_left(mid);
    try_jump(mid, to);
  }

  // Base case: no continuation — emit only if we captured at least one piece
  if (!any && captured) list.push({orig, cur, captured, false});
}

void draughts::generate_captures(const Position& pos, MoveList& list) {
  const Color us = pos.side_to_move;
  const Bitboard enemy = pos.pieces(~us);
  const Bitboard empty = pos.empty_sq;

  Bitboard men = pos.bb[us][MAN];
  while (men) {
    Square from = pop_lsb(men);
    capture_from(from, from, enemy, empty, 0, false, us, list);
  }

  Bitboard kings = pos.bb[us][KING];
  while (kings) {
    Square from = pop_lsb(kings);
    capture_from(from, from, enemy, empty, 0, true, us, list);
  }
}

void draughts::generate_quiets(const Position& pos, MoveList& list) {
  const Color us = pos.side_to_move;
  const Bitboard men = pos.bb[us][MAN];
  const Bitboard kings = pos.bb[us][KING];
  const Bitboard empty = pos.empty_sq;
  const Bitboard promo = (us == BLACK) ? BLACK_PROMO_RANK : WHITE_PROMO_RANK;

  using FwdFn = Square (*)(Bitboard);

  // Enumerate destination squares; recover source via the reverse shift
  auto emit = [&](Bitboard dests, bool is_man, FwdFn from_of) {
    while (dests) {
      Square to = pop_lsb(dests);
      list.push({from_of(sq_bb(to)), to, 0, is_man && bool(sq_bb(to) & promo)});
    }
  };

  if (us == BLACK) {
    emit(shift_up_right(men) & empty, true,
         [](Bitboard b) { return lsb(shift_down_left(b)); });
    emit(shift_up_left(men) & empty, true,
         [](Bitboard b) { return lsb(shift_down_right(b)); });
  } else {
    emit(shift_down_right(men) & empty, true,
         [](Bitboard b) { return lsb(shift_up_left(b)); });
    emit(shift_down_left(men) & empty, true,
         [](Bitboard b) { return lsb(shift_up_right(b)); });
  }
  // Kings move in all four directions regardless of color; they never promote
  emit(shift_up_right(kings) & empty, false,
       [](Bitboard b) { return lsb(shift_down_left(b)); });
  emit(shift_up_left(kings) & empty, false,
       [](Bitboard b) { return lsb(shift_down_right(b)); });
  emit(shift_down_right(kings) & empty, false,
       [](Bitboard b) { return lsb(shift_up_left(b)); });
  emit(shift_down_left(kings) & empty, false,
       [](Bitboard b) { return lsb(shift_up_right(b)); });
}

void draughts::generate_moves(const Position& pos, MoveList& list) {
  generate_captures(pos, list);
  if (list.count > 0) return;
  generate_quiets(pos, list);
}

bool draughts::has_captures(const Position& pos) {
  const Color us = pos.side_to_move;
  const Bitboard men = pos.bb[us][MAN];
  const Bitboard kings = pos.bb[us][KING];
  const Bitboard enemy = pos.pieces(~us);
  const Bitboard empty = pos.empty_sq;

  using FwdFn = Bitboard (*)(Bitboard);

  // Apply the shift twice: once to reach the enemy, once to reach the landing
  // square
  auto can_jump = [&](FwdFn fwd, Bitboard pieces) -> bool {
    return (fwd(fwd(pieces) & enemy) & empty) != 0;
  };

  if (us == BLACK) {
    if (can_jump(shift_up_right, men)) return true;
    if (can_jump(shift_up_left, men)) return true;
  } else {
    if (can_jump(shift_down_right, men)) return true;
    if (can_jump(shift_down_left, men)) return true;
  }
  if (can_jump(shift_up_right, kings)) return true;
  if (can_jump(shift_up_left, kings)) return true;
  if (can_jump(shift_down_right, kings)) return true;
  if (can_jump(shift_down_left, kings)) return true;
  return false;
}

bool draughts::has_legal_moves(const Position& pos) {
  if (has_captures(pos)) return true;

  const Color us = pos.side_to_move;
  const Bitboard men = pos.bb[us][MAN];
  const Bitboard kings = pos.bb[us][KING];
  const Bitboard empty = pos.empty_sq;

  if (us == BLACK) {
    if ((shift_up_right(men) | shift_up_left(men)) & empty) return true;
  } else {
    if ((shift_down_right(men) | shift_down_left(men)) & empty) return true;
  }
  return (shift_all(kings) & empty) != 0;
}
