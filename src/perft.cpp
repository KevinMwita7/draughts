#include "draughts/perft.h"

#include <ostream>

#include "draughts/movegen.h"

namespace draughts {

uint64_t perft(Position& pos, int depth) {
  if (depth <= 0) return 1;

  MoveList list;
  generate_moves(pos, list);

  uint64_t nodes = 0;
  for (Move& m : list) {
    pos.do_move(m);
    nodes += perft(pos, depth - 1);
    pos.undo_move(m);
  }
  return nodes;
}

uint64_t perft_divide(Position& pos, int depth, std::ostream& out) {
  if (depth < 1) {
    out << "\nTotal: 0\n";
    return 0;
  }

  MoveList list;
  generate_moves(pos, list);

  uint64_t total = 0;
  for (Move& m : list) {
    pos.do_move(m);
    uint64_t count = perft(pos, depth - 1);
    pos.undo_move(m);

    // PDN notation: squares are 1-indexed; '-' for quiet, 'x' for capture
    char sep = m.captured ? 'x' : '-';
    out << (m.from + 1) << sep << (m.to + 1) << ": " << count << '\n';
    total += count;
  }
  out << "\nTotal: " << total << '\n';
  return total;
}

}  // namespace draughts
