#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include "position.h"

namespace draughts {

struct Evaluator;

struct SearchParams {
  int max_depth = 64;
  int time_ms = 0;         // 0 = unlimited
  uint64_t max_nodes = 0;  // 0 = unlimited
  std::atomic<bool>* stop_signal = nullptr;
  std::atomic<int>* switch_time_ms = nullptr;  // set by ponderhit; search polls this
};

// Search result

struct SearchResult {
  Move best_move = NULL_MOVE;
  Score score = SCORE_NONE;
  int depth = 0;
  uint64_t nodes = 0;
  int elapsed_ms = 0;
};

// Per-iteration callback (for UCI-style "info" output)
// Called after each completed depth with the result so far.
using InfoCallback = std::function<void(const SearchResult&)>;

// Entry point
// Iterative-deepening alpha-beta search.
// pos is modified during search (do_move / undo_move) but restored on return.
// game_history is the sequence of position hashes already reached in the
// real game (see TextProtocol::history); it seeds the search's own
// repetition check so a line that revisits an earlier real-game position is
// recognized as a draw during search, not just after the move is played.
SearchResult search(Position& pos, const SearchParams& params, Evaluator& eval,
                    InfoCallback cb = nullptr,
                    const std::vector<uint64_t>& game_history = {});

}  // namespace draughts
