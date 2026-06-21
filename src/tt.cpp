#include "draughts/tt.h"

#include <algorithm>
#include <bit>
#include <cstring>

namespace draughts {

static constexpr uint32_t MAX_ENTRIES =
    1u << 24;  // 16 M entries × 12 B sizeof(TTEntry) = 192 MB

TranspositionTable::TranspositionTable(uint32_t n_entries) {
  resize(n_entries);
}

void TranspositionTable::resize(uint32_t n_entries) {
  n_entries = std::bit_ceil(n_entries);
  n_entries = std::min(n_entries, MAX_ENTRIES);
  table_.assign(n_entries, TTEntry{});
  mask_ =
      n_entries -
      1;  // power-of-2: (n-1) is all 1s, so hash & mask_ == hash % n_entries
}

void TranspositionTable::clear() {
  std::fill(table_.begin(), table_.end(), TTEntry{});
}

void TranspositionTable::new_search() { ++current_gen_; }

bool TranspositionTable::probe(uint64_t hash, TTEntry* out) const {
  const TTEntry& e = table_[hash & mask_];
  if (e.key == 0 || e.key != static_cast<uint32_t>(hash >> 32)) return false;
  *out = e;
  return true;
}

void TranspositionTable::store(uint64_t hash, Score score, Move best_move,
                               uint8_t depth, TTFlag flag) {
  TTEntry& e = table_[hash & mask_];
  uint32_t key32 = static_cast<uint32_t>(hash >> 32);

  // Protect deeper entries only within the current generation.
  // Old-generation entries are always evictable.
  if (e.key != 0 && e.key == key32 && e.gen == current_gen_ && e.depth > depth)
    return;

  e.key = key32;
  e.score = static_cast<int16_t>(score);
  e.move_from = best_move.from;
  e.move_to = best_move.to;
  e.depth = depth;
  e.flag = flag;
  e.gen = current_gen_;
}

int TranspositionTable::hashfull() const {
  const auto sample =
      static_cast<uint32_t>(std::min<size_t>(1000, table_.size()));
  int filled = 0;
  for (uint32_t i = 0; i < sample; ++i)
    if (table_[i].key != 0) ++filled;
  return static_cast<int>(filled * 1000 / sample);
}

}  // namespace draughts
