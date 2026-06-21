#pragma once
#include "position.h"

namespace draughts {

// Abstract evaluator interface.
// Returns a score in centipawns from side_to_move's perspective:
//   positive = good for side_to_move, negative = bad.
struct Evaluator {
  virtual ~Evaluator() = default;
  virtual Score evaluate(const Position& pos) const = 0;
};
}  // namespace draughts
