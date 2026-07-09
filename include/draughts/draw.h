#pragma once
#include <cstdint>
#include <vector>

#include "position.h"

namespace draughts {

// Pragmatic flat no-progress rule (simplification of the real 25-move /
// 16-move reduced-material variants used in international draughts).
constexpr uint8_t MAX_REVERSIBLE_PLIES = 40;

inline bool is_reversible_draw(const Position& pos) noexcept {
  return pos.reversible >= MAX_REVERSIBLE_PLIES;
}

inline int count_occurrences(uint64_t hash,
                              const std::vector<uint64_t>& history) noexcept {
  int n = 0;
  for (uint64_t h : history)
    if (h == hash) ++n;
  return n;
}

}  // namespace draughts
