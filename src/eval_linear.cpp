#include "draughts/eval_linear.h"

#include <algorithm>
#include <cstdlib>

#include "draughts/bitboard.h"
#include "draughts/movegen.h"
#include "draughts/position.h"

namespace draughts {

namespace {

constexpr Bitboard CENTER_4 = sq_bb(13) | sq_bb(14) | sq_bb(17) | sq_bb(18);
constexpr Bitboard CENTER_8 = RANK[3] | RANK[4];

// Forward progress of square s for color c: row for BLACK, 7-row for WHITE.
int forward_row(Square s, Color c) noexcept {
  int row = s / 4;
  return c == BLACK ? row : 7 - row;
}

}  // namespace

static const int kDefaultWeights[LinearEval::NUM_FEATURES] = {
    100,  //  0  man_diff
    150,  //  1  king_diff
      3,  //  2  advancement
     10,  //  3  back_rank
     15,  //  4  center_4
      5,  //  5  center_8
      2,  //  6  mobility
      0,  //  7  tempo
      0,  //  8  own_men
      0,  //  9  opp_men
      0,  // 10  own_kings
      0,  // 11  opp_kings
      0,  // 12  lone_king_dist
      1,  // 13  attack
      8,  // 14  runaway
      0,  // 15  (reserved)
};

LinearEval::LinearEval() {
  std::copy(kDefaultWeights, kDefaultWeights + NUM_FEATURES, weights);
}

LinearEval::LinearEval(const int* w) {
  std::copy(w, w + NUM_FEATURES, weights);
}

void LinearEval::extract_features(const Position& pos, int* out) const {
  const Color own = pos.side_to_move;
  const Color opp = ~own;

  // 0: man_diff
  out[0] = pos.count(own, MAN) - pos.count(opp, MAN);

  // 1: king_diff
  out[1] = pos.count(own, KING) - pos.count(opp, KING);

  // 2: advancement — sum of own men's forward row
  {
    int adv = 0;
    Bitboard men = pos.men(own);
    while (men) adv += forward_row(pop_lsb(men), own);
    out[2] = adv;
  }

  // 3: back_rank — own pieces on home back rank
  {
    Bitboard home = (own == BLACK) ? RANK[0] : RANK[7];
    out[3] = popcount(pos.pieces(own) & home);
  }

  // 4 & 5: center control
  out[4] = popcount(pos.pieces(own) & CENTER_4);
  out[5] = popcount(pos.pieces(own) & CENTER_8);

  // 6: mobility — quiet moves available (0 when captures are mandatory)
  {
    MoveList qm;
    generate_quiets(pos, qm);
    out[6] = qm.count;
  }

  // 7: tempo
  out[7] = (own == BLACK) ? 1 : -1;

  // 8-11: raw piece counts
  out[8]  = pos.count(own, MAN);
  out[9]  = pos.count(opp, MAN);
  out[10] = pos.count(own, KING);
  out[11] = pos.count(opp, KING);

  // 12: lone_king_dist — min Chebyshev distance from own lone king to any enemy
  {
    int dist = 0;
    if (pos.count(own, KING) == 1 && pos.count(own, MAN) == 0) {
      Square ks = lsb(pos.kings(own));
      int kr = Board32to64[ks] / 8, kc = Board32to64[ks] % 8;
      int min_d = 16;
      Bitboard enemies = pos.pieces(opp);
      while (enemies) {
        Square es = pop_lsb(enemies);
        int er = Board32to64[es] / 8, ec = Board32to64[es] % 8;
        int d = std::max(std::abs(kr - er), std::abs(kc - ec));
        if (d < min_d) min_d = d;
      }
      dist = min_d;
    }
    out[12] = dist;
  }

  // 13: attack — diagonal squares own pieces threaten (excluding own-occupied)
  {
    Bitboard men   = pos.men(own);
    Bitboard kings = pos.kings(own);
    Bitboard attacked;
    if (own == BLACK)
      attacked = shift_up_right(men) | shift_up_left(men);
    else
      attacked = shift_down_right(men) | shift_down_left(men);
    attacked |= shift_all(kings);
    out[13] = popcount(attacked & ~pos.pieces(own));
  }

  // 14: runaway — row of most-advanced own man (0 if no men)
  {
    int run = 0;
    Bitboard men = pos.men(own);
    while (men) {
      int r = forward_row(pop_lsb(men), own);
      if (r > run) run = r;
    }
    out[14] = run;
  }

  // 15: reserved
  out[15] = 0;
}

Score LinearEval::evaluate(const Position& pos) const {
  int feats[NUM_FEATURES];
  extract_features(pos, feats);
  Score score = 0;
  for (int i = 0; i < NUM_FEATURES; ++i) score += weights[i] * feats[i];
  return score;
}

}  // namespace draughts
