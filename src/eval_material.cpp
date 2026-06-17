#include "draughts/eval_material.h"
#include "draughts/position.h"

namespace draughts {

Score MaterialEval::evaluate(const Position& pos) const {
    Color own = pos.side_to_move;
    Color opp = ~own;
    int man_diff  = pos.count(own, MAN)  - pos.count(opp, MAN);
    int king_diff = pos.count(own, KING) - pos.count(opp, KING);
    return man_value * man_diff + king_value * king_diff;
}

}
