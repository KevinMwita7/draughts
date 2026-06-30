#pragma once
#include "eval.h"

namespace draughts {

// Samuel-style linear evaluator: weighted sum of handcrafted board features.
// Inspired by Arthur Samuel's 1959 checkers program.  Weights are stored as
// integer centipawns and can be tuned offline.
//
// All features are computed from side_to_move's perspective.
//
// Feature index constants (passed to extract_features):
//   0  man_diff        — own men minus opponent men
//   1  king_diff       — own kings minus opponent kings
//   2  advancement     — sum of own men's row index (forward pressure)
//   3  back_rank       — own pieces on home back rank (king safety)
//   4  center_4        — own pieces on the central 4 squares (13,14,17,18)
//   5  center_8        — own pieces on the inner 8 squares (RANK[3] | RANK[4])
//   6  mobility        — number of quiet moves available (0 when captures mandatory)
//   7  tempo           — 1 if side_to_move == BLACK, -1 if WHITE (side bias)
//   8  own_men         — raw own man count
//   9  opp_men         — raw opponent man count
//  10  own_kings       — raw own king count
//  11  opp_kings       — raw opponent king count
//  12  lone_king_dist  — min Chebyshev distance from own lone king to nearest enemy
//  13  attack          — diagonal squares own pieces threaten (excluding own-occupied)
//  14  runaway         — row of most-advanced own man (passed-piece analog; 0 if no men)
//  15  (reserved)
//
// Default weights make material the dominant signal (man=100, king=150) with
// smaller positional bonuses: center_4=15, back_rank=10, runaway=8, center_8=5,
// advancement=3, mobility=2, attack=1.  Raw counts (8-11), tempo, and
// lone_king_dist are zeroed by default as context-dependent tuning levers.

struct LinearEval : Evaluator {
  static constexpr int NUM_FEATURES = 16;

  int weights[NUM_FEATURES];

  LinearEval();                       // default weights (hand-tuned)
  explicit LinearEval(const int* w);  // supply external weight array

  Score evaluate(const Position& pos) const override;

  // Fill out[0..NUM_FEATURES-1] with raw feature values.
  // Features are always from side_to_move's perspective.
  void extract_features(const Position& pos, int* out) const;
};

}  // namespace draughts
