#include "draughts/perft.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "draughts/movegen.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

class PerftTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

  static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                           Bitboard w_kings, Color side) {
    Position pos{};
    pos.bb[BLACK][MAN] = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN] = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.rebuild_derived();
    pos.side_to_move = side;
    pos.hash = compute_hash(pos);
    pos.ply = 0;
    pos.reversible = 0;
    return pos;
  }
};

// perft

TEST_F(PerftTest, Depth0_AlwaysReturnsOne) {
  Position start_position = Position::start_position();
  EXPECT_EQ(perft(start_position, 0), 1u);
}

TEST_F(PerftTest, Depth1_StartPosition) {
  // Same count that generate_moves reports for the opening position.
  Position start_position = Position::start_position();
  EXPECT_EQ(perft(start_position, 1), 7u);
}

TEST_F(PerftTest, Depth2_StartPosition) {
  Position start_position = Position::start_position();
  // Black's 7 moves × white's 7 replies (no interaction between the two
  // front rows yet) = 49.
  EXPECT_EQ(perft(start_position, 2), 49u);
}

TEST_F(PerftTest, Depth3_StartPosition) {
  Position start_position = Position::start_position();
  EXPECT_EQ(perft(start_position, 3), 302u);
}

TEST_F(PerftTest, TerminalPosition_ReturnsZero) {
  // BLACK has no pieces → no legal moves → every subtree is empty.
  Position pos = make_pos(0, 0, sq_bb(20), 0, BLACK);
  EXPECT_EQ(perft(pos, 1), 0u);
  EXPECT_EQ(perft(pos, 2), 0u);
}

TEST_F(PerftTest, ForcedCapture_Depth1IsOne) {
  // Black at sq 9, white at sq 13 → only legal move is 9x13→18.
  // Depth 1: exactly one leaf.
  Position pos = make_pos(sq_bb(9), 0, sq_bb(13), 0, BLACK);
  EXPECT_EQ(perft(pos, 1), 1u);
}

TEST_F(PerftTest, ForcedCapture_Depth2IsZero) {
  // After 9x18 the white piece is gone; white (now to move) has no pieces.
  Position pos = make_pos(sq_bb(9), 0, sq_bb(13), 0, BLACK);
  EXPECT_EQ(perft(pos, 2), 0u);
}

TEST_F(PerftTest, DoesNotMutatePosition) {
  Position pos = Position::start_position();
  Position before = pos;
  perft(pos, 3);
  EXPECT_EQ(pos.bb[BLACK][MAN], before.bb[BLACK][MAN]);
  EXPECT_EQ(pos.bb[BLACK][KING], before.bb[BLACK][KING]);
  EXPECT_EQ(pos.bb[WHITE][MAN], before.bb[WHITE][MAN]);
  EXPECT_EQ(pos.bb[WHITE][KING], before.bb[WHITE][KING]);
  EXPECT_EQ(pos.side_to_move, before.side_to_move);
  EXPECT_EQ(pos.hash, before.hash);
  EXPECT_EQ(pos.ply, before.ply);
  EXPECT_EQ(pos.reversible, before.reversible);
}

// perft_divide

class PerftDivideTest : public PerftTest {};

TEST_F(PerftDivideTest, ReturnValueMatchesPerft) {
  Position pos = Position::start_position();
  std::ostringstream out;
  Position copy = pos;
  EXPECT_EQ(perft_divide(pos, 3, out), perft(copy, 3));
}

TEST_F(PerftDivideTest, Depth0_ReturnsZero) {
  std::ostringstream out;
  Position start_position = Position::start_position();
  EXPECT_EQ(perft_divide(start_position, 0, out), 0u);
}

TEST_F(PerftDivideTest, Depth0_OutputShowsZeroTotal) {
  std::ostringstream out;
  Position start_position = Position::start_position();
  perft_divide(start_position, 0, out);
  EXPECT_NE(out.str().find("Total: 0"), std::string::npos);
}

TEST_F(PerftDivideTest, OutputContainsTotalLine) {
  std::ostringstream out;
  Position start_position = Position::start_position();
  perft_divide(start_position, 1, out);
  EXPECT_NE(out.str().find("Total: 7"), std::string::npos);
}

TEST_F(PerftDivideTest, OneMoveLinePerRootMove) {
  std::ostringstream out;
  Position start_position = Position::start_position();
  perft_divide(start_position, 1, out);

  // Count non-blank lines that are not the Total line.
  int count = 0;
  std::istringstream iss(out.str());
  std::string line;
  while (std::getline(iss, line))
    if (!line.empty() && line.find("Total:") == std::string::npos) ++count;

  EXPECT_EQ(count, 7);
}

TEST_F(PerftDivideTest, QuietMoveUsesDash) {
  // All root moves from the start position are quiet.
  std::ostringstream out;
  Position start_position = Position::start_position();
  perft_divide(start_position, 1, out);
  std::string s = out.str();
  EXPECT_NE(s.find('-'), std::string::npos);
  EXPECT_EQ(s.find('x'), std::string::npos);
}

TEST_F(PerftDivideTest, CaptureUsesX) {
  // Black at sq 9, white at sq 13 → only root move is a capture.
  Position pos = make_pos(sq_bb(9), 0, sq_bb(13), 0, BLACK);
  std::ostringstream out;
  perft_divide(pos, 1, out);
  std::string s = out.str();
  // The move line must contain 'x' and must use PDN squares (10 and 19).
  EXPECT_NE(s.find("10x19"), std::string::npos);
}

TEST_F(PerftDivideTest, TerminalPosition_ReturnsZeroAndShowsTotal) {
  Position pos = make_pos(0, 0, sq_bb(20), 0, BLACK);
  std::ostringstream out;
  EXPECT_EQ(perft_divide(pos, 1, out), 0u);
  EXPECT_NE(out.str().find("Total: 0"), std::string::npos);
}

TEST_F(PerftDivideTest, DoesNotMutatePosition) {
  Position pos = Position::start_position();
  Position before = pos;
  std::ostringstream out;
  perft_divide(pos, 2, out);
  EXPECT_EQ(pos.side_to_move, before.side_to_move);
  EXPECT_EQ(pos.hash, before.hash);
  EXPECT_EQ(pos.ply, before.ply);
}
