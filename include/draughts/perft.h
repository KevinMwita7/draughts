#pragma once
#include "position.h"
#include <cstdint>
#include <iosfwd>

namespace draughts {

// Count leaf nodes at exactly 'depth' plies from pos.
// Useful for verifying move generation correctness against known values.
uint64_t perft(Position& pos, int depth);

// perft_divide: prints the move and its sub-tree node count for every root
// move, then prints the total.  Returns the total node count.
uint64_t perft_divide(Position& pos, int depth, std::ostream& out);

} // namespace draughts
