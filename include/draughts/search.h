#pragma once
#include "position.h"
#include <cstdint>
#include <functional>

namespace draughts {

struct Evaluator;

// ---- Search parameters ------------------------------------------------------

struct SearchParams {
    int      max_depth = 64;     // 0 = unlimited
    int      time_ms   = 0;      // 0 = unlimited
    uint64_t max_nodes = 0;      // 0 = unlimited
};

// ---- Search result ----------------------------------------------------------

struct SearchResult {
    Move     best_move  = NULL_MOVE;
    Score    score      = SCORE_NONE;
    int      depth      = 0;
    uint64_t nodes      = 0;
    int      elapsed_ms = 0;
};

// ---- Per-iteration callback (for UCI-style "info" output) -------------------
// Called after each completed depth with the result so far.
using InfoCallback = std::function<void(const SearchResult&)>;

// ---- Entry point ------------------------------------------------------------
// Iterative-deepening alpha-beta search.
// pos is modified during search (do_move / undo_move) but restored on return.
SearchResult search(Position&          pos,
                    const SearchParams& params,
                    Evaluator&          eval,
                    InfoCallback        cb = nullptr);

} // namespace draughts
