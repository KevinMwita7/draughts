#include "draughts/eval_linear.h"

#include <gtest/gtest.h>

#include "draughts/bitboard.h"
#include "draughts/position.h"

using namespace draughts;

static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                         Bitboard w_kings, Color side) {
  Position pos{};
  pos.bb[BLACK][MAN]  = b_men;
  pos.bb[BLACK][KING] = b_kings;
  pos.bb[WHITE][MAN]  = w_men;
  pos.bb[WHITE][KING] = w_kings;
  pos.rebuild_derived();
  pos.side_to_move = side;
  return pos;
}

// Extract a single feature using a default-constructed LinearEval.
static int feat(const Position& pos, int idx) {
  LinearEval ev;
  int f[LinearEval::NUM_FEATURES];
  ev.extract_features(pos, f);
  return f[idx];
}

// ---- Constructor ------------------------------------------------------------

TEST(LinearEvalConstructor, DefaultManDiffWeight) {
  EXPECT_EQ(LinearEval{}.weights[0], 100);
}

TEST(LinearEvalConstructor, DefaultKingDiffWeight) {
  EXPECT_EQ(LinearEval{}.weights[1], 150);
}

TEST(LinearEvalConstructor, DefaultRawCountWeightsAreZero) {
  LinearEval ev;
  EXPECT_EQ(ev.weights[8],  0);
  EXPECT_EQ(ev.weights[9],  0);
  EXPECT_EQ(ev.weights[10], 0);
  EXPECT_EQ(ev.weights[11], 0);
}

TEST(LinearEvalConstructor, CustomWeightsCopied) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[0] = 42;
  w[3] = 7;
  LinearEval ev(w);
  EXPECT_EQ(ev.weights[0], 42);
  EXPECT_EQ(ev.weights[3], 7);
  EXPECT_EQ(ev.weights[1], 0);  // unset slots remain 0
}

// ---- Feature 0 & 1: material ------------------------------------------------

TEST(LinearEvalFeatures, ManDiffOwnAhead) {
  // 2 BLACK men, 1 WHITE man, BLACK to move → man_diff = +1
  Position pos = make_pos(sq_bb(4) | sq_bb(5), 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(feat(pos, 0), 1);
}

TEST(LinearEvalFeatures, ManDiffOppAhead) {
  Position pos = make_pos(sq_bb(4), 0, sq_bb(20) | sq_bb(21), 0, BLACK);
  EXPECT_EQ(feat(pos, 0), -1);
}

TEST(LinearEvalFeatures, KingDiffWhitePerspective) {
  // 2 BLACK kings, 1 WHITE king; WHITE to move → king_diff = 1-2 = -1
  Position pos = make_pos(0, sq_bb(12) | sq_bb(13), 0, sq_bb(18), WHITE);
  EXPECT_EQ(feat(pos, 1), -1);
}

// ---- Feature 2: advancement -------------------------------------------------

TEST(LinearEvalFeatures, AdvancementBlackManRow3) {
  // sq 12 = row 3 → forward_row = 3
  Position pos = make_pos(sq_bb(12), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 2), 3);
}

TEST(LinearEvalFeatures, AdvancementBlackTwoMen) {
  // sq 4 row 1, sq 20 row 5: sum = 1 + 5 = 6
  Position pos = make_pos(sq_bb(4) | sq_bb(20), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 2), 6);
}

TEST(LinearEvalFeatures, AdvancementWhiteManRow4) {
  // sq 16 row 4: forward_row = 7-4 = 3
  Position pos = make_pos(0, 0, sq_bb(16), 0, WHITE);
  EXPECT_EQ(feat(pos, 2), 3);
}

TEST(LinearEvalFeatures, AdvancementKingsNotCounted) {
  Position pos = make_pos(0, sq_bb(13), 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 2), 0);
}

// ---- Feature 3: back_rank ---------------------------------------------------

TEST(LinearEvalFeatures, BackRankBlackPieceOnRow0) {
  // sq 1 is on row 0 (BLACK home back rank = RANK[0])
  Position pos = make_pos(sq_bb(1), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 3), 1);
}

TEST(LinearEvalFeatures, BackRankWhitePieceOnRow7) {
  // sq 30 is on row 7 (WHITE home back rank = RANK[7])
  Position pos = make_pos(0, 0, sq_bb(30), 0, WHITE);
  EXPECT_EQ(feat(pos, 3), 1);
}

TEST(LinearEvalFeatures, BackRankPieceNotOnHomeRank) {
  Position pos = make_pos(sq_bb(12), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 3), 0);
}

TEST(LinearEvalFeatures, BackRankCountsMixedPiecesOnRow0) {
  // 2 BLACK men (sq 0, 1) + 1 BLACK king (sq 2) on row 0 → 3
  Position pos = make_pos(sq_bb(0) | sq_bb(1), sq_bb(2), 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 3), 3);
}

// ---- Feature 4 & 5: center control ------------------------------------------

TEST(LinearEvalFeatures, Center4ManOnSq13) {
  Position pos = make_pos(sq_bb(13), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 4), 1);
}

TEST(LinearEvalFeatures, Center4TwoPieces) {
  // sq 13 and sq 17 are both in CENTER_4
  Position pos = make_pos(sq_bb(13) | sq_bb(17), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 4), 2);
}

TEST(LinearEvalFeatures, Center8IncludesCenter4) {
  Position pos = make_pos(sq_bb(13), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 5), 1);
}

TEST(LinearEvalFeatures, Center8NotCenter4) {
  // sq 12 is in RANK[3] (center_8) but not in CENTER_4 {13,14,17,18}
  Position pos = make_pos(sq_bb(12), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 4), 0);
  EXPECT_EQ(feat(pos, 5), 1);
}

TEST(LinearEvalFeatures, Center4WhitePerspective) {
  // WHITE man on sq 18 (CENTER_4), WHITE to move
  Position pos = make_pos(0, 0, sq_bb(18), 0, WHITE);
  EXPECT_EQ(feat(pos, 4), 1);
}

// ---- Feature 6: mobility ----------------------------------------------------

TEST(LinearEvalFeatures, MobilityBlackManLeftEdge) {
  // sq 8 (LEFT_EDGE, row 2): only up-right to sq 12 → 1 quiet move
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 6), 1);
}

TEST(LinearEvalFeatures, MobilityBlackManTwoMoves) {
  // sq 9 (row 2, not edge): up-right to sq 13, up-left to sq 12 → 2
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 6), 2);
}

TEST(LinearEvalFeatures, MobilityZeroWhenDestinationOccupied) {
  // sq 8 can only go to sq 12; sq 12 occupied → 0
  Position pos = make_pos(sq_bb(8), 0, sq_bb(12), 0, BLACK);
  EXPECT_EQ(feat(pos, 6), 0);
}

// ---- Feature 7: tempo -------------------------------------------------------

TEST(LinearEvalFeatures, TempoBlackToMove) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(feat(pos, 7), 1);
}

TEST(LinearEvalFeatures, TempoWhiteToMove) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, WHITE);
  EXPECT_EQ(feat(pos, 7), -1);
}

// ---- Features 8-11: raw piece counts ----------------------------------------

TEST(LinearEvalFeatures, RawCountsBlackPerspective) {
  // 2 BLACK men, 1 BLACK king, 3 WHITE men, 0 WHITE kings; BLACK to move
  Position pos = make_pos(sq_bb(4) | sq_bb(5), sq_bb(13),
                          sq_bb(20) | sq_bb(21) | sq_bb(22), 0, BLACK);
  LinearEval ev;
  int f[LinearEval::NUM_FEATURES];
  ev.extract_features(pos, f);
  EXPECT_EQ(f[8],  2);
  EXPECT_EQ(f[9],  3);
  EXPECT_EQ(f[10], 1);
  EXPECT_EQ(f[11], 0);
}

TEST(LinearEvalFeatures, RawCountsWhitePerspective) {
  // Same position, WHITE to move: own/opp swap
  Position pos = make_pos(sq_bb(4) | sq_bb(5), sq_bb(13),
                          sq_bb(20) | sq_bb(21) | sq_bb(22), 0, WHITE);
  LinearEval ev;
  int f[LinearEval::NUM_FEATURES];
  ev.extract_features(pos, f);
  EXPECT_EQ(f[8],  3);
  EXPECT_EQ(f[9],  2);
  EXPECT_EQ(f[10], 0);
  EXPECT_EQ(f[11], 1);
}

// ---- Feature 12: lone_king_dist ---------------------------------------------

TEST(LinearEvalFeatures, LoneKingDistZeroWhenMultipleKings) {
  Position pos = make_pos(0, sq_bb(13) | sq_bb(14), sq_bb(20), 0, BLACK);
  EXPECT_EQ(feat(pos, 12), 0);
}

TEST(LinearEvalFeatures, LoneKingDistZeroWhenHasMen) {
  // Own has 1 king + 1 man: condition not met
  Position pos = make_pos(sq_bb(4), sq_bb(13), sq_bb(20), 0, BLACK);
  EXPECT_EQ(feat(pos, 12), 0);
}

TEST(LinearEvalFeatures, LoneKingDistChebyshev) {
  // BLACK lone king sq 13: Board32to64[13]=35 → 8x8 (4,3)
  // WHITE man sq 18:       Board32to64[18]=28 → 8x8 (3,4)
  // Chebyshev = max(|4-3|, |3-4|) = 1
  Position pos = make_pos(0, sq_bb(13), sq_bb(18), 0, BLACK);
  EXPECT_EQ(feat(pos, 12), 1);
}

TEST(LinearEvalFeatures, LoneKingDistMinOverMultipleEnemies) {
  // BLACK lone king sq 13: 8x8 (4,3)
  // WHITE sq 18: 8x8 (3,4) → dist 1
  // WHITE sq 28: Board32to64[28]=1 → 8x8 (0,1) → dist max(4,2)=4
  // min = 1
  Position pos = make_pos(0, sq_bb(13), sq_bb(18) | sq_bb(28), 0, BLACK);
  EXPECT_EQ(feat(pos, 12), 1);
}

// ---- Feature 13: attack -----------------------------------------------------

TEST(LinearEvalFeatures, AttackBlackManTwoSquares) {
  // sq 9 (row 2, even, not LEFT_EDGE): up-right sq 13, up-left sq 12 → 2
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 13), 2);
}

TEST(LinearEvalFeatures, AttackBlackManLeftEdgeOneSquare) {
  // sq 8 (LEFT_EDGE): only up-right sq 12 → 1
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 13), 1);
}

TEST(LinearEvalFeatures, AttackWhiteManTwoSquares) {
  // sq 20 (row 5, odd): down-right sq 17, down-left sq 16 → 2
  Position pos = make_pos(0, 0, sq_bb(20), 0, WHITE);
  EXPECT_EQ(feat(pos, 13), 2);
}

TEST(LinearEvalFeatures, AttackKingFourSquares) {
  // WHITE king sq 13 (row 3, odd): attacks sq 18, 17, 10, 9 → 4
  Position pos = make_pos(0, 0, 0, sq_bb(13), WHITE);
  EXPECT_EQ(feat(pos, 13), 4);
}

TEST(LinearEvalFeatures, AttackExcludesOwnPieces) {
  // BLACK men at sq 9 and sq 13.
  // Attacked: sq 13 (from sq 9), sq 12 (from sq 9), sq 18 (from sq 13), sq 17 (from sq 13).
  // Own pieces {9,13} excluded → {12, 18, 17} = 3
  Position pos = make_pos(sq_bb(9) | sq_bb(13), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 13), 3);
}

// ---- Feature 14: runaway ----------------------------------------------------

TEST(LinearEvalFeatures, RunawayZeroWithNoMen) {
  Position pos = make_pos(0, sq_bb(13), 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 14), 0);
}

TEST(LinearEvalFeatures, RunawayBlackRow5) {
  // sq 20 row 5 → runaway = 5
  Position pos = make_pos(sq_bb(20), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 14), 5);
}

TEST(LinearEvalFeatures, RunawayBlackTakesMax) {
  // sq 4 row 1, sq 20 row 5 → max = 5
  Position pos = make_pos(sq_bb(4) | sq_bb(20), 0, 0, 0, BLACK);
  EXPECT_EQ(feat(pos, 14), 5);
}

TEST(LinearEvalFeatures, RunawayWhiteForwardRow) {
  // WHITE man sq 8 (row 2): forward_row = 7-2 = 5 → runaway = 5
  Position pos = make_pos(0, 0, sq_bb(8), 0, WHITE);
  EXPECT_EQ(feat(pos, 14), 5);
}

// ---- Feature 15: reserved ---------------------------------------------------

TEST(LinearEvalFeatures, ReservedAlwaysZero) {
  EXPECT_EQ(feat(Position::start_position(), 15), 0);
}

// ---- evaluate ---------------------------------------------------------------

TEST(LinearEvalEvaluate, AllZeroWeightsScoresZero) {
  int w[LinearEval::NUM_FEATURES] = {};
  LinearEval ev(w);
  EXPECT_EQ(ev.evaluate(Position::start_position()), 0);
  EXPECT_EQ(ev.evaluate(make_pos(sq_bb(4), 0, sq_bb(20), 0, BLACK)), 0);
}

TEST(LinearEvalEvaluate, ManDiffWeightOnly) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[0] = 100;
  LinearEval ev(w);
  // 2 BLACK men vs 1 WHITE man, BLACK to move → 100*(2-1) = 100
  Position pos = make_pos(sq_bb(4) | sq_bb(5), 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(ev.evaluate(pos), 100);
}

TEST(LinearEvalEvaluate, PerspectiveFlipManDiff) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[0] = 100;
  LinearEval ev(w);
  // Same position, WHITE to move: own=WHITE (1 man) vs opp=BLACK (2 men) → -100
  Position pos = make_pos(sq_bb(4) | sq_bb(5), 0, sq_bb(20), 0, WHITE);
  EXPECT_EQ(ev.evaluate(pos), -100);
}

TEST(LinearEvalEvaluate, KingDiffWeightOnly) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[1] = 50;
  LinearEval ev(w);
  // 2 BLACK kings, 1 WHITE king; BLACK to move → 50*(2-1) = 50
  Position pos = make_pos(0, sq_bb(12) | sq_bb(13), 0, sq_bb(18), BLACK);
  EXPECT_EQ(ev.evaluate(pos), 50);
}

TEST(LinearEvalEvaluate, AdvancementWeightOnly) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[2] = 1;
  LinearEval ev(w);
  // BLACK man sq 12 (row 3): advancement = 3 → score = 3
  Position pos = make_pos(sq_bb(12), 0, 0, 0, BLACK);
  EXPECT_EQ(ev.evaluate(pos), 3);
}

TEST(LinearEvalEvaluate, MultipleWeightSum) {
  int w[LinearEval::NUM_FEATURES] = {};
  w[0] = 100;  // man_diff
  w[2] = 1;    // advancement
  LinearEval ev(w);
  // BLACK men: sq 12 (row 3) + sq 4 (row 1), WHITE man: sq 20
  // man_diff = 2-1 = 1 → ×100 = 100
  // advancement = 3+1 = 4 → ×1 = 4
  // total = 104
  Position pos = make_pos(sq_bb(12) | sq_bb(4), 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(ev.evaluate(pos), 104);
}
