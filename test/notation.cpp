#include "draughts/notation.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "draughts/bitboard.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

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

static const char* START_PDN =
    "B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12";

// ============================================================================
// parse_pdn_position
// ============================================================================

class ParsePdnPositionTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }
};

TEST_F(ParsePdnPositionTest, StartPosition_BlackMen) {
  EXPECT_EQ(parse_pdn_position(START_PDN).bb[BLACK][MAN], Bitboard{0x00000FFF});
}

TEST_F(ParsePdnPositionTest, StartPosition_WhiteMen) {
  EXPECT_EQ(parse_pdn_position(START_PDN).bb[WHITE][MAN], Bitboard{0xFFF00000});
}

TEST_F(ParsePdnPositionTest, StartPosition_NoKings) {
  Position pos = parse_pdn_position(START_PDN);
  EXPECT_EQ(pos.bb[BLACK][KING], Bitboard{0});
  EXPECT_EQ(pos.bb[WHITE][KING], Bitboard{0});
}

TEST_F(ParsePdnPositionTest, StartPosition_SideToMoveBlack) {
  EXPECT_EQ(parse_pdn_position(START_PDN).side_to_move, BLACK);
}

TEST_F(ParsePdnPositionTest, WhiteToMove) {
  Position pos = parse_pdn_position(
      "W:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12");
  EXPECT_EQ(pos.side_to_move, WHITE);
}

TEST_F(ParsePdnPositionTest, BlackKing_InKingBitboard) {
  // "K5" → black king on PDN 5 = sq 4
  Position pos = parse_pdn_position("B:W20:BK5");
  EXPECT_EQ(pos.bb[BLACK][KING], sq_bb(4));
  EXPECT_EQ(pos.bb[BLACK][MAN], Bitboard{0});
}

TEST_F(ParsePdnPositionTest, WhiteKing_InKingBitboard) {
  // "WK29" → white king on PDN 29 = sq 28
  Position pos = parse_pdn_position("B:WK29:B1");
  EXPECT_EQ(pos.bb[WHITE][KING], sq_bb(28));
  EXPECT_EQ(pos.bb[WHITE][MAN], Bitboard{0});
}

TEST_F(ParsePdnPositionTest, MixedKingsAndMen_CorrectBitboards) {
  // Black: man on PDN 1 (sq 0), king on PDN 5 (sq 4)
  Position pos = parse_pdn_position("B:W20:B1,K5");
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(0));
  EXPECT_EQ(pos.bb[BLACK][KING], sq_bb(4));
}

TEST_F(ParsePdnPositionTest, OccupiedDerived) {
  // Black man PDN 1 (sq 0), white man PDN 20 (sq 19)
  Position pos = parse_pdn_position("B:W20:B1");
  EXPECT_EQ(pos.occupied, sq_bb(0) | sq_bb(19));
}

TEST_F(ParsePdnPositionTest, EmptySqDerived) {
  Position pos = parse_pdn_position("B:W20:B1");
  EXPECT_EQ(pos.empty_sq, ~(sq_bb(0) | sq_bb(19)));
}

TEST_F(ParsePdnPositionTest, Hash_ConsistentWithComputeHash) {
  Position pos = parse_pdn_position(START_PDN);
  EXPECT_EQ(pos.hash, compute_hash(pos));
}

// ============================================================================
// to_pdn_position
// ============================================================================

class ToPdnPositionTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }
};

TEST_F(ToPdnPositionTest, StartPosition_ExactString) {
  EXPECT_EQ(to_pdn_position(Position::start_position()), START_PDN);
}

TEST_F(ToPdnPositionTest, WhiteToMove_StartsWithW) {
  Position pos = make_pos(sq_bb(0), 0, sq_bb(19), 0, WHITE);
  EXPECT_EQ(to_pdn_position(pos).substr(0, 2), "W:");
}

TEST_F(ToPdnPositionTest, BlackKing_KPrefix) {
  // Black king on sq 4 (PDN 5) → output must contain "K5" in B section
  Position pos = make_pos(0, sq_bb(4), sq_bb(19), 0, BLACK);
  EXPECT_NE(to_pdn_position(pos).find("K5"), std::string::npos);
}

TEST_F(ToPdnPositionTest, WhiteKing_KPrefix) {
  // White king on sq 28 (PDN 29) → output must contain "K29" in W section
  Position pos = make_pos(sq_bb(0), 0, 0, sq_bb(28), BLACK);
  EXPECT_NE(to_pdn_position(pos).find("K29"), std::string::npos);
}

TEST_F(ToPdnPositionTest, RoundTrip_BitboardsMatch) {
  Position orig = make_pos(sq_bb(0) | sq_bb(1), sq_bb(4), sq_bb(19) | sq_bb(20),
                           sq_bb(28), BLACK);
  Position rt = parse_pdn_position(to_pdn_position(orig));
  EXPECT_EQ(rt.bb[BLACK][MAN], orig.bb[BLACK][MAN]);
  EXPECT_EQ(rt.bb[BLACK][KING], orig.bb[BLACK][KING]);
  EXPECT_EQ(rt.bb[WHITE][MAN], orig.bb[WHITE][MAN]);
  EXPECT_EQ(rt.bb[WHITE][KING], orig.bb[WHITE][KING]);
}

TEST_F(ToPdnPositionTest, RoundTrip_SideToMoveMatches) {
  Position orig = make_pos(sq_bb(0), 0, sq_bb(19), 0, WHITE);
  EXPECT_EQ(parse_pdn_position(to_pdn_position(orig)).side_to_move, WHITE);
}

// ============================================================================
// parse_move
// ============================================================================

class ParseMoveTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }
};

TEST_F(ParseMoveTest, QuietMove_FromSquare) {
  // Black man sq 8 (PDN 9, LEFT_EDGE row 2): only quiet move is to sq 12 (PDN
  // 13)
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_EQ(parse_move("9-13", pos).from, Square{8});
}

TEST_F(ParseMoveTest, QuietMove_ToSquare) {
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_EQ(parse_move("9-13", pos).to, Square{12});
}

TEST_F(ParseMoveTest, QuietMove_NoCaptured) {
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_EQ(parse_move("9-13", pos).captured, Bitboard{0});
}

TEST_F(ParseMoveTest, QuietMove_NoPromotion) {
  Position pos = make_pos(sq_bb(8), 0, 0, 0, BLACK);
  EXPECT_FALSE(parse_move("9-13", pos).promotion);
}

TEST_F(ParseMoveTest, QuietPromotion_FlagSet) {
  // Black man sq 24 (PDN 25, LEFT_EDGE row 6) → sq 28 (PDN 29,
  // BLACK_PROMO_RANK)
  Position pos = make_pos(sq_bb(24), 0, 0, 0, BLACK);
  EXPECT_TRUE(parse_move("25-29", pos).promotion);
}

TEST_F(ParseMoveTest, SingleCapture_FromSquare) {
  // Black sq 0 (PDN 1), white sq 4 (PDN 5); lands on sq 9 (PDN 10)
  Position pos = make_pos(sq_bb(0), 0, sq_bb(4), 0, BLACK);
  EXPECT_EQ(parse_move("1x10", pos).from, Square{0});
}

TEST_F(ParseMoveTest, SingleCapture_ToSquare) {
  Position pos = make_pos(sq_bb(0), 0, sq_bb(4), 0, BLACK);
  EXPECT_EQ(parse_move("1x10", pos).to, Square{9});
}

TEST_F(ParseMoveTest, SingleCapture_CapturedBitboard) {
  Position pos = make_pos(sq_bb(0), 0, sq_bb(4), 0, BLACK);
  EXPECT_EQ(parse_move("1x10", pos).captured, sq_bb(4));
}

TEST_F(ParseMoveTest, CapturePromotion_FlagSet) {
  // Black sq 20 (PDN 21), white sq 25 (PDN 26); lands on sq 29 (PDN 30, promo
  // rank)
  Position pos = make_pos(sq_bb(20), 0, sq_bb(25), 0, BLACK);
  Move m = parse_move("21x30", pos);
  EXPECT_TRUE(m.promotion);
  EXPECT_EQ(m.captured, sq_bb(25));
}

TEST_F(ParseMoveTest, MultiJump_ToSquare) {
  // Black sq 1 (PDN 2), white sq 5 (PDN 6) and sq 14 (PDN 15)
  // Chain: sq 1 → over sq 5 → sq 10 (PDN 11) → over sq 14 → sq 19 (PDN 20)
  Position pos = make_pos(sq_bb(1), 0, sq_bb(5) | sq_bb(14), 0, BLACK);
  EXPECT_EQ(parse_move("2x11x20", pos).to, Square{19});
}

TEST_F(ParseMoveTest, MultiJump_CapturedBitboard) {
  Position pos = make_pos(sq_bb(1), 0, sq_bb(5) | sq_bb(14), 0, BLACK);
  EXPECT_EQ(parse_move("2x11x20", pos).captured, sq_bb(5) | sq_bb(14));
}

// ============================================================================
// move_to_string
// ============================================================================

TEST(MoveToStringTest, QuietMove_DashSeparator) {
  // Move from sq 4 (PDN 5) to sq 8 (PDN 9): "5-9"
  EXPECT_EQ(move_to_string({4, 8, 0, false}), "5-9");
}

TEST(MoveToStringTest, Capture_CrossSeparator) {
  // Black sq 0 (PDN 1) captures sq 4, lands sq 9 (PDN 10): "1x10"
  EXPECT_EQ(move_to_string({0, 9, sq_bb(4), false}), "1x10");
}

TEST(MoveToStringTest, Promotion_DashSeparator) {
  // Quiet promotion: same dash format as quiet moves
  EXPECT_EQ(move_to_string({24, 28, 0, true}), "25-29");
}

// ============================================================================
// parse_pdn
// ============================================================================

class ParsePdnTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }
};

TEST_F(ParsePdnTest, SingleBlackMove_ReturnsOneMove) {
  // PDN 11-15: sq 10 → sq 14 (standard opening)
  EXPECT_EQ(parse_pdn("1. 11-15").size(), 1u);
}

TEST_F(ParsePdnTest, SingleBlackMove_IsCorrect) {
  auto moves = parse_pdn("1. 11-15");
  ASSERT_EQ(moves.size(), 1u);
  EXPECT_EQ(moves[0].from, Square{10});
  EXPECT_EQ(moves[0].to, Square{14});
}

TEST_F(ParsePdnTest, TwoMoves_ReturnsTwoMoves) {
  // Black 11-15, white 23-19
  EXPECT_EQ(parse_pdn("1. 11-15 23-19").size(), 2u);
}

TEST_F(ParsePdnTest, TwoMoves_BothCorrect) {
  auto moves = parse_pdn("1. 11-15 23-19");
  ASSERT_EQ(moves.size(), 2u);
  // Black: sq 10 (PDN 11) → sq 14 (PDN 15)
  EXPECT_EQ(moves[0].from, Square{10});
  EXPECT_EQ(moves[0].to, Square{14});
  // White: sq 22 (PDN 23) → sq 18 (PDN 19)
  EXPECT_EQ(moves[1].from, Square{22});
  EXPECT_EQ(moves[1].to, Square{18});
}
