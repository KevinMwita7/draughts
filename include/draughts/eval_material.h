#pragma once
#include "eval.h"

namespace draughts {

// Simple material evaluator: man_value * (own_men - opp_men) + king_value *
// (own_kings - opp_kings)
struct MaterialEval : Evaluator {
  int man_value = 100;
  int king_value = 150;

  Score evaluate(const Position& pos) const override;
};
}  // namespace draughts
