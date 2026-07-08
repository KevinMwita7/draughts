#include "draughts/search.h"

#include <chrono>
#include <vector>

#include "draughts/draw.h"
#include "draughts/eval.h"
#include "draughts/movegen.h"

namespace draughts {

namespace {

struct State {
  const SearchParams& params;
  Evaluator& eval;
  uint64_t nodes = 0;
  bool stopped = false;
  std::vector<uint64_t> path;  // hashes along the current search line
};

Score negamax(Position& pos, int depth, Score alpha, Score beta, State& st) {
  if (st.params.stop_signal &&
      st.params.stop_signal->load(std::memory_order_relaxed)) {
    st.stopped = true;
    return SCORE_NONE;
  }
  if (st.params.max_nodes > 0 && st.nodes >= st.params.max_nodes) {
    st.stopped = true;
    return SCORE_NONE;
  }
  ++st.nodes;

  if (is_reversible_draw(pos)) return SCORE_DRAW;
  if (count_occurrences(pos.hash, st.path) >= 2) return SCORE_DRAW;

  if (depth == 0) return st.eval.evaluate(pos);

  MoveList moves;
  generate_moves(pos, moves);

  if (moves.empty()) return SCORE_LOSS;

  for (int i = 0; i < moves.count; ++i) {
    pos.do_move(moves[i]);
    st.path.push_back(pos.hash);
    Score score = -negamax(pos, depth - 1, -beta, -alpha, st);
    st.path.pop_back();
    pos.undo_move(moves[i]);

    if (st.stopped) return SCORE_NONE;

    if (score > alpha) {
      alpha = score;
      if (alpha >= beta) break;
    }
  }
  return alpha;
}

}  // namespace

SearchResult search(Position& pos, const SearchParams& params, Evaluator& eval,
                    InfoCallback cb) {
  SearchResult result;

  MoveList root_moves;
  generate_moves(pos, root_moves);
  if (root_moves.empty()) {
    result.score = SCORE_LOSS;
    return result;
  }

  int max_depth = (params.max_depth > 0) ? params.max_depth : 64;
  auto start = std::chrono::steady_clock::now();
  State st{params, eval};
  bool switched = false;
  std::chrono::steady_clock::time_point switch_start;

  for (int depth = 1; depth <= max_depth && !st.stopped; ++depth) {
    Score alpha = -SCORE_INF;
    Move best = root_moves[0];

    for (int i = 0; i < root_moves.count; ++i) {
      pos.do_move(root_moves[i]);
      st.path.push_back(pos.hash);
      Score score = -negamax(pos, depth - 1, -SCORE_INF, -alpha, st);
      st.path.pop_back();
      pos.undo_move(root_moves[i]);

      if (st.stopped) break;

      if (score > alpha) {
        alpha = score;
        best = root_moves[i];
      }
    }

    auto now = std::chrono::steady_clock::now();
    result.elapsed_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count());
    result.nodes = st.nodes;

    if (!st.stopped) {
      result.best_move = best;
      result.score = alpha;
      result.depth = depth;
    }

    if (cb) cb(result);

    // Ponderhit: switch_time_ms goes 0 → real_ms on the main thread.
    // Capture switch_start = now so the budget is measured from ponderhit.
    if (!switched && params.switch_time_ms != nullptr) {
      int sw = params.switch_time_ms->load(std::memory_order_acquire);
      if (sw > 0) {
        switched = true;
        switch_start = now;
      }
    }

    if (switched) {
      auto sw_ms = static_cast<int>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - switch_start)
              .count());
      if (sw_ms >= params.switch_time_ms->load(std::memory_order_relaxed))
        break;
    } else if (params.time_ms > 0 && result.elapsed_ms >= params.time_ms) {
      break;
    }

    if (result.score >= SCORE_WIN &&
        (params.switch_time_ms == nullptr || switched))
      break;
  }

  return result;
}

}  // namespace draughts
