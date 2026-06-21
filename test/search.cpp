#include "draughts/search.h"

#include <gtest/gtest.h>

#include "draughts/bitboard.h"
#include "draughts/eval_material.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

// ============================================================================
// SearchParams defaults
// ============================================================================

TEST(SearchParamsTest, DefaultMaxDepthIs64) {
  EXPECT_EQ(SearchParams{}.max_depth, 64);
}

TEST(SearchParamsTest, DefaultTimeMsIsZero) {
  EXPECT_EQ(SearchParams{}.time_ms, 0);
}

TEST(SearchParamsTest, DefaultMaxNodesIsZero) {
  EXPECT_EQ(SearchParams{}.max_nodes, uint64_t{0});
}

// ============================================================================
// SearchResult defaults
// ============================================================================

TEST(SearchResultTest, DefaultBestMoveIsNullMove) {
  EXPECT_EQ(SearchResult{}.best_move, NULL_MOVE);
}

TEST(SearchResultTest, DefaultScoreIsScoreNone) {
  EXPECT_EQ(SearchResult{}.score, SCORE_NONE);
}

TEST(SearchResultTest, DefaultDepthIsZero) {
  EXPECT_EQ(SearchResult{}.depth, 0);
}

TEST(SearchResultTest, DefaultNodesIsZero) {
  EXPECT_EQ(SearchResult{}.nodes, uint64_t{0});
}

// ============================================================================
// Fixture
// ============================================================================

class SearchTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

  MaterialEval eval;

  static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                           Bitboard w_kings, Color side) {
    Position pos{};
    pos.bb[BLACK][MAN] = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN] = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.occupied = b_men | b_kings | w_men | w_kings;
    pos.empty_sq = ~pos.occupied;
    pos.side_to_move = side;
    pos.hash = compute_hash(pos);
    return pos;
  }
};

// ============================================================================
// Terminal position (side_to_move has no legal moves)
// ============================================================================

TEST_F(SearchTest, TerminalPosition_ScoreIsLoss) {
  // BLACK man on promo rank, no enemies — man has no legal move.
  Position pos = make_pos(sq_bb(28), 0, 0, 0, BLACK);
  SearchParams params;
  params.max_depth = 4;
  EXPECT_EQ(search(pos, params, eval).score, SCORE_LOSS);
}

TEST_F(SearchTest, TerminalPosition_BestMoveIsNullMove) {
  Position pos = make_pos(sq_bb(28), 0, 0, 0, BLACK);
  SearchParams params;
  params.max_depth = 4;
  EXPECT_EQ(search(pos, params, eval).best_move, NULL_MOVE);
}

TEST_F(SearchTest, TerminalPosition_NodesIsZero) {
  Position pos = make_pos(sq_bb(28), 0, 0, 0, BLACK);
  SearchParams params;
  params.max_depth = 4;
  EXPECT_EQ(search(pos, params, eval).nodes, uint64_t{0});
}

TEST_F(SearchTest, TerminalPosition_WhiteSide_ScoreIsLoss) {
  // WHITE man on promo rank (row 0), no legal move.
  Position pos = make_pos(0, 0, sq_bb(0), 0, WHITE);
  SearchParams params;
  params.max_depth = 4;
  EXPECT_EQ(search(pos, params, eval).score, SCORE_LOSS);
}

// ============================================================================
// Non-terminal position — basic sanity
// ============================================================================

TEST_F(SearchTest, NonTerminal_BestMoveIsNotNullMove) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 1;
  EXPECT_NE(search(pos, params, eval).best_move, NULL_MOVE);
}

TEST_F(SearchTest, NonTerminal_NodesIsPositive) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 1;
  EXPECT_GT(search(pos, params, eval).nodes, uint64_t{0});
}

TEST_F(SearchTest, NonTerminal_DepthAtLeastOne) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  EXPECT_GE(search(pos, params, eval).depth, 1);
}

TEST_F(SearchTest, Depth1_ReportedDepthIsOne) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 1;
  EXPECT_EQ(search(pos, params, eval).depth, 1);
}

// ============================================================================
// Win in 1 — forced capture wins immediately
// ============================================================================

// BLACK man sq 5 captures WHITE man sq 9, lands on sq 12.
// After capture WHITE has no pieces → terminal loss for WHITE.
// At depth 2 BLACK should find the unique winning move.

TEST_F(SearchTest, WinInOne_FindsCapturingMove) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  Move expected{5, 12, sq_bb(9), false};
  EXPECT_EQ(search(pos, params, eval).best_move, expected);
}

TEST_F(SearchTest, WinInOne_ScoreIsPositive) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  EXPECT_GT(search(pos, params, eval).score, 0);
}

TEST_F(SearchTest, WinInOne_ScoreIsWin) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  EXPECT_EQ(search(pos, params, eval).score, SCORE_WIN);
}

// ============================================================================
// Score perspective
// ============================================================================

TEST_F(SearchTest, Perspective_BlackAheadScoresPositiveFromBlackSide) {
  // 2 BLACK men vs 1 WHITE man — material favours BLACK.
  Position pos = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 1;
  EXPECT_GT(search(pos, params, eval).score, 0);
}

TEST_F(SearchTest, Perspective_WhiteAheadScoresNegativeFromBlackSide) {
  // 1 BLACK man vs 2 WHITE men — material favours WHITE.
  Position pos = make_pos(sq_bb(1), 0, sq_bb(20) | sq_bb(21), 0, BLACK);
  SearchParams params;
  params.max_depth = 1;
  EXPECT_LT(search(pos, params, eval).score, 0);
}

TEST_F(SearchTest, Perspective_FlipsWhenSideChanges) {
  // Same material imbalance, opposite sides to move.
  Position black_stm = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, BLACK);
  Position white_stm = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, WHITE);
  SearchParams params;
  params.max_depth = 1;
  Score s_black = search(black_stm, params, eval).score;
  Score s_white = search(white_stm, params, eval).score;
  EXPECT_GT(s_black, 0);
  EXPECT_LT(s_white, 0);
}

// ============================================================================
// InfoCallback
// ============================================================================

TEST_F(SearchTest, Callback_CalledAtLeastOnce) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  int calls = 0;
  search(pos, params, eval, [&](const SearchResult&) { ++calls; });
  EXPECT_GE(calls, 1);
}

TEST_F(SearchTest, Callback_ReceivedDepthIsPositive) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  int last_depth = 0;
  search(pos, params, eval,
         [&](const SearchResult& r) { last_depth = r.depth; });
  EXPECT_GE(last_depth, 1);
}

TEST_F(SearchTest, Callback_DepthIncreasesMonotonically) {
  Position pos = Position::start_position();
  SearchParams params;
  params.max_depth = 3;
  int prev = 0;
  bool monotonic = true;
  search(pos, params, eval, [&](const SearchResult& r) {
    if (r.depth <= prev) monotonic = false;
    prev = r.depth;
  });
  EXPECT_TRUE(monotonic);
}

TEST_F(SearchTest, Callback_ReceivedBestMoveIsNotNullMove) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  SearchParams params;
  params.max_depth = 2;
  Move last_move = NULL_MOVE;
  search(pos, params, eval,
         [&](const SearchResult& r) { last_move = r.best_move; });
  EXPECT_NE(last_move, NULL_MOVE);
}

// ============================================================================
// Node limit
// ============================================================================

TEST_F(SearchTest, MaxNodes_NodeCountDoesNotExceedLimit) {
  Position pos = Position::start_position();
  SearchParams params;
  params.max_depth = 64;
  params.max_nodes = 50;
  SearchResult result = search(pos, params, eval);
  EXPECT_LE(result.nodes, params.max_nodes);
}

// ============================================================================
// Position is restored after search
// ============================================================================

TEST_F(SearchTest, PositionRestoredAfterSearch) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(20), 0, BLACK);
  Bitboard b_before = pos.bb[BLACK][MAN];
  Bitboard w_before = pos.bb[WHITE][MAN];
  Color stm_before = pos.side_to_move;
  SearchParams params;
  params.max_depth = 3;
  search(pos, params, eval);
  EXPECT_EQ(pos.bb[BLACK][MAN], b_before);
  EXPECT_EQ(pos.bb[WHITE][MAN], w_before);
  EXPECT_EQ(pos.side_to_move, stm_before);
}

TEST_F(SearchTest, PositionRestoredAfterNodeLimitStop) {
  Position pos = Position::start_position();
  Bitboard b_before = pos.bb[BLACK][MAN];
  SearchParams params;
  params.max_depth = 64;
  params.max_nodes = 20;
  search(pos, params, eval);
  EXPECT_EQ(pos.bb[BLACK][MAN], b_before);
  EXPECT_EQ(pos.side_to_move, BLACK);
}
