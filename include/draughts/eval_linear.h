#pragma once
#include "eval.h"

namespace draughts {

// Samuel-style linear evaluator: weighted sum of handcrafted board features.
// Inspired by Arthur Samuel's 1959 checkers program.  Weights are stored as
// integer centipawns and can be tuned offline.
//
// Feature index constants (passed to extract_features):
//   0  man_diff        — own men minus opponent men
//   1  king_diff       — own kings minus opponent kings
//   2  advancement     — sum of own men's row index (forward pressure)
//   3  back_rank       — own pieces on home back rank (king safety)
//   4  center_4        — own pieces on the central 4 squares
//   5  center_8        — own pieces on the inner 8 squares
//   6  mobility        — number of quiet moves available
//   7  tempo           — 1 if side_to_move == BLACK, -1 if WHITE (side bias)
//   8  own_men         — raw own man count
//   9  opp_men         — raw opponent man count
//  10  own_kings       — raw own king count
//  11  opp_kings       — raw opponent king count
//  12  lone_king_dist  — distance between lone king and nearest enemy (endgame)
//  13  attack          — squares attacked by own pieces
//  14  runaway         — most-advanced own man row (passed-piece analog)
//  15  (reserved)

struct LinearEval : Evaluator {
    static constexpr int NUM_FEATURES = 16;

    int weights[NUM_FEATURES];

    LinearEval();                            // default weights (hand-tuned)
    explicit LinearEval(const int* w);       // supply external weight array

    Score evaluate(const Position& pos) const override;

    // Fill out[0..NUM_FEATURES-1] with raw feature values.
    // Features are always from side_to_move's perspective.
    void extract_features(const Position& pos, int* out) const;
};

} // namespace draughts
