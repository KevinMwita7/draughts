#include "draughts/movegen.h"

#include <gtest/gtest.h>

#include "draughts/bitboard.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

class GenerateMovesTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

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
    pos.ply = 0;
    pos.reversible = 0;
    return pos;
  }

  static MoveList gen(const Position& pos) {
    MoveList list;
    generate_moves(pos, list);
    return list;
  }

  static bool has_move(const MoveList& list, Move m) {
    for (const Move& mv : list)
      if (mv == m) return true;
    return false;
  }
};

// ---- no pieces for side to move ---------------------------------------------

TEST_F(GenerateMovesTest, NoPiecesForSideToMove_ZeroMoves) {
  Position pos = make_pos(0, 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(gen(pos).count, 0);
}

// ---- black man quiet moves --------------------------------------------------

TEST_F(GenerateMovesTest, BlackManMidBoard_TwoQuietMoves) {
  // sq 9 (row 2, even): up-right → 13, up-left → 12
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 2);
  EXPECT_TRUE(has_move(list, {9, 13, 0, false}));
  EXPECT_TRUE(has_move(list, {9, 12, 0, false}));
}

TEST_F(GenerateMovesTest, BlackManOnLeftEdge_OneQuietMove) {
  // sq 8 is in LEFT_EDGE (col 0, even row): no up-left neighbour
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {8, 12, 0, false}));
}

TEST_F(GenerateMovesTest, BlackManOnRightEdgeOddRow_OneQuietMove) {
  // sq 7 is in RIGHT_EDGE (col 7, odd row): no up-right neighbour
  Position pos = make_pos(sq_bb(7), 0, 0, 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {7, 11, 0, false}));
}

// ---- white man quiet moves --------------------------------------------------

TEST_F(GenerateMovesTest, WhiteManMidBoard_TwoQuietMoves) {
  // sq 21 (row 5, odd): down-right → 18, down-left → 17
  Position pos = make_pos(0, 0, sq_bb(21), 0, WHITE);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 2);
  EXPECT_TRUE(has_move(list, {21, 18, 0, false}));
  EXPECT_TRUE(has_move(list, {21, 17, 0, false}));
}

// ---- king quiet moves -------------------------------------------------------

TEST_F(GenerateMovesTest, KingMidBoard_FourQuietMoves) {
  // Black king on sq 13 (row 3, odd): all four diagonals open
  // up-right → 18, up-left → 17, down-right → 10, down-left → 9
  Position pos = make_pos(0, sq_bb(13), 0, 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 4);
  EXPECT_TRUE(has_move(list, {13, 18, 0, false}));
  EXPECT_TRUE(has_move(list, {13, 17, 0, false}));
  EXPECT_TRUE(has_move(list, {13, 10, 0, false}));
  EXPECT_TRUE(has_move(list, {13, 9, 0, false}));
}

// ---- captures are mandatory -------------------------------------------------

TEST_F(GenerateMovesTest, CapturesMandatory_QuietMoveSuppressed) {
  // Black on sq 9; white on sq 13 (blocks quiet up-right but can be captured →
  // sq 18). Quiet move to sq 12 is available but must be suppressed because a
  // capture exists.
  Position pos = make_pos(sq_bb(9), 0, sq_bb(13), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {9, 18, sq_bb(13), false}));
}

// ---- single captures --------------------------------------------------------

TEST_F(GenerateMovesTest, BlackManSingleCapture_CorrectMove) {
  // sq 5 (odd) jumps over sq 9 (even) via up-left, lands on sq 12 (odd)
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {5, 12, sq_bb(9), false}));
}

TEST_F(GenerateMovesTest, WhiteManSingleCapture_CorrectMove) {
  // sq 21 (odd) jumps over sq 17 (even) via down-left, lands on sq 12 (odd)
  Position pos = make_pos(sq_bb(17), 0, sq_bb(21), 0, WHITE);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {21, 12, sq_bb(17), false}));
}

TEST_F(GenerateMovesTest, KingCapture_DownwardDirection) {
  // Black king on sq 17 (even); white man on sq 13 (down-right); land on sq 10
  // (odd) shift_down_right(17)=13, shift_down_right(13)=10
  Position pos = make_pos(0, sq_bb(17), sq_bb(13), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {17, 10, sq_bb(13), false}));
}

// ---- multi-jump capture -----------------------------------------------------

TEST_F(GenerateMovesTest, MultiJump_FullChainInSingleMove) {
  // Black on sq 1 (even); white on sq 5 (odd) and sq 14 (odd).
  // Jump sequence: sq 1 → over sq 5 → sq 10 → over sq 14 → sq 19
  Position pos = make_pos(sq_bb(1), 0, sq_bb(5) | sq_bb(14), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_TRUE(has_move(list, {1, 19, sq_bb(5) | sq_bb(14), false}));
}

// ---- promotion --------------------------------------------------------------

TEST_F(GenerateMovesTest, BlackManQuietPromotion_FlagIsSet) {
  // sq 25 (row 6, even): up-right → 29 and up-left → 28, both on
  // BLACK_PROMO_RANK
  Position pos = make_pos(sq_bb(25), 0, 0, 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_TRUE(has_move(list, {25, 29, 0, true}));
  EXPECT_TRUE(has_move(list, {25, 28, 0, true}));
}

TEST_F(GenerateMovesTest, BlackManCapturePromotion_FlagIsSet) {
  // Black on sq 20 (odd); white on sq 25 (even); land on sq 29
  // (BLACK_PROMO_RANK) shift_up_right(20)=25, shift_up_right(25)=29
  Position pos = make_pos(sq_bb(20), 0, sq_bb(25), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 1);
  EXPECT_TRUE(has_move(list, {20, 29, sq_bb(25), true}));
}

// ---- two distinct capture choices -------------------------------------------

TEST_F(GenerateMovesTest, TwoDistinctCaptureChoices_BothReturned) {
  // Black on sq 9; white on sq 12 (up-left) and sq 13 (up-right).
  // Captures: sq 9 over sq 12 → sq 16, and sq 9 over sq 13 → sq 18.
  Position pos = make_pos(sq_bb(9), 0, sq_bb(12) | sq_bb(13), 0, BLACK);
  MoveList list = gen(pos);
  EXPECT_EQ(list.count, 2);
  EXPECT_TRUE(has_move(list, {9, 16, sq_bb(12), false}));
  EXPECT_TRUE(has_move(list, {9, 18, sq_bb(13), false}));
}

// ---- start position ---------------------------------------------------------

TEST_F(GenerateMovesTest, StartPosition_BlackHasSevenMoves) {
  // Only row-2 men (sq 8-11) can move.
  // sq 8 → 1 move (LEFT_EDGE); sq 9,10,11 → 2 moves each. Total = 7.
  MoveList list = gen(Position::start_position());
  EXPECT_EQ(list.count, 7);
}
