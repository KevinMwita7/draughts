#include <gtest/gtest.h>

#include "draughts/bitboard.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

class DoMoveTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

  static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                           Bitboard w_kings, Color side, uint16_t ply = 0,
                           uint8_t reversible = 0) {
    Position pos{};
    pos.bb[BLACK][MAN]  = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN]  = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.occupied        = b_men | b_kings | w_men | w_kings;
    pos.empty_sq        = ~pos.occupied;
    pos.side_to_move    = side;
    pos.hash            = compute_hash(pos);
    pos.ply             = ply;
    pos.reversible      = reversible;
    return pos;
  }
};

// ---- piece bitboards --------------------------------------------------------

TEST_F(DoMoveTest, BlackManMovesFromSourceToDestination) {
  // sq 9 (r2 even) -> sq 13 (r3 odd) via shift_up_right (+4)
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(13));
}

TEST_F(DoMoveTest, WhiteManMovesFromSourceToDestination) {
  // sq 21 (r5 odd) -> sq 17 (r4 even) via shift_down_left (-4)
  Position pos = make_pos(0, 0, sq_bb(21), 0, WHITE);
  Move m{21, 17, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[WHITE][MAN], sq_bb(17));
}

TEST_F(DoMoveTest, BlackKingMovesFromSourceToDestination) {
  // sq 13 (r3 odd) -> sq 17 (r4 even) via shift_up_left (+4)
  Position pos = make_pos(0, sq_bb(13), 0, 0, BLACK);
  Move m{13, 17, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[BLACK][KING], sq_bb(17));
  EXPECT_EQ(pos.bb[BLACK][MAN], Bitboard{0});
}

TEST_F(DoMoveTest, QuietMoveDoesNotAffectOpponentBitboards) {
  Position pos = make_pos(sq_bb(9), 0, sq_bb(21), 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[WHITE][MAN], sq_bb(21));
}

// ---- captures ---------------------------------------------------------------

TEST_F(DoMoveTest, BlackCaptureRemovesOpponentManFromBitboard) {
  // sq 5 (r1 odd) captures sq 9 (r2 even), lands on sq 12 (r3 odd)
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(12));
  EXPECT_EQ(pos.bb[WHITE][MAN], Bitboard{0});
}

TEST_F(DoMoveTest, WhiteCaptureRemovesOpponentManFromBitboard) {
  // sq 21 (r5 odd) captures sq 17 (r4 even), lands on sq 12 (r3 odd)
  Position pos = make_pos(sq_bb(17), 0, sq_bb(21), 0, WHITE);
  Move m{21, 12, sq_bb(17), false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[WHITE][MAN], sq_bb(12));
  EXPECT_EQ(pos.bb[BLACK][MAN], Bitboard{0});
}

TEST_F(DoMoveTest, MultipleCapturesRemoveAllOpponentPieces) {
  // sq 1 (r0 even) captures sq 5 (r1), intermediate sq 10 (r2),
  // captures sq 14 (r3), lands on sq 19 (r4)
  Position pos = make_pos(sq_bb(1), 0, sq_bb(5) | sq_bb(14), 0, BLACK);
  Move m{1, 19, sq_bb(5) | sq_bb(14), false};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(19));
  EXPECT_EQ(pos.bb[WHITE][MAN], Bitboard{0});
}

// ---- promotion --------------------------------------------------------------

TEST_F(DoMoveTest, BlackManOnPromoRankBecomesKing) {
  // sq 25 (r6 even) -> sq 29 (r7 BLACK_PROMO_RANK)
  Position pos = make_pos(sq_bb(25), 0, 0, 0, BLACK);
  Move m{25, 29, 0, true};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN],  Bitboard{0});
  EXPECT_EQ(pos.bb[BLACK][KING], sq_bb(29));
}

TEST_F(DoMoveTest, WhiteManOnPromoRankBecomesKing) {
  // sq 4 (r1 odd) -> sq 0 (r0 WHITE_PROMO_RANK)
  Position pos = make_pos(0, 0, sq_bb(4), 0, WHITE);
  Move m{4, 0, 0, true};
  pos.do_move(m);
  EXPECT_EQ(pos.bb[WHITE][MAN],  Bitboard{0});
  EXPECT_EQ(pos.bb[WHITE][KING], sq_bb(0));
}

// ---- occupied ---------------------------------------------------------------

TEST_F(DoMoveTest, OccupiedUpdatedAfterQuietMove) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.occupied, sq_bb(13));
}

TEST_F(DoMoveTest, OccupiedClearsCapturedSquares) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  EXPECT_EQ(pos.occupied, sq_bb(12));
}

// ---- side to move -----------------------------------------------------------

TEST_F(DoMoveTest, SideToMoveFlipsBlackToWhite) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.side_to_move, WHITE);
}

TEST_F(DoMoveTest, SideToMoveFlipsWhiteToBlack) {
  Position pos = make_pos(0, 0, sq_bb(21), 0, WHITE);
  Move m{21, 17, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.side_to_move, BLACK);
}

// ---- hash -------------------------------------------------------------------

TEST_F(DoMoveTest, HashConsistentAfterQuietMove) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.hash, compute_hash(pos));
}

TEST_F(DoMoveTest, HashConsistentAfterCapture) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  EXPECT_EQ(pos.hash, compute_hash(pos));
}

TEST_F(DoMoveTest, HashConsistentAfterPromotion) {
  Position pos = make_pos(sq_bb(25), 0, 0, 0, BLACK);
  Move m{25, 29, 0, true};
  pos.do_move(m);
  EXPECT_EQ(pos.hash, compute_hash(pos));
}

// ---- counters ---------------------------------------------------------------

TEST_F(DoMoveTest, PlyIncrements) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK, 3);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.ply, uint16_t{4});
}

TEST_F(DoMoveTest, ReversibleIncrementsOnQuietMove) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK, 0, 7);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  EXPECT_EQ(pos.reversible, uint8_t{8});
}

TEST_F(DoMoveTest, ReversibleResetsToZeroOnCapture) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK, 0, 5);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  EXPECT_EQ(pos.reversible, uint8_t{0});
}

// ============================================================================
// undo_move
// ============================================================================

class UndoMoveTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

  static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                           Bitboard w_kings, Color side, uint16_t ply = 0,
                           uint8_t reversible = 0) {
    Position pos{};
    pos.bb[BLACK][MAN]  = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN]  = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.occupied        = b_men | b_kings | w_men | w_kings;
    pos.empty_sq        = ~pos.occupied;
    pos.side_to_move    = side;
    pos.hash            = compute_hash(pos);
    pos.ply             = ply;
    pos.reversible      = reversible;
    return pos;
  }
};

// ---- piece bitboards --------------------------------------------------------

TEST_F(UndoMoveTest, QuietMove_BitboardRestored) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(9));
}

TEST_F(UndoMoveTest, CapturedMan_Restored) {
  // Black man sq 5 captures white man sq 9, lands on sq 12
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(5));
  EXPECT_EQ(pos.bb[WHITE][MAN], sq_bb(9));
}

TEST_F(UndoMoveTest, CapturedKing_Restored) {
  // Black man sq 5 captures white KING sq 9, lands on sq 12
  Position pos = make_pos(sq_bb(5), 0, 0, sq_bb(9), BLACK);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN],  sq_bb(5));
  EXPECT_EQ(pos.bb[WHITE][KING], sq_bb(9));
  EXPECT_EQ(pos.bb[WHITE][MAN],  Bitboard{0});
}

TEST_F(UndoMoveTest, Promotion_Reverted) {
  // Black man sq 25 -> sq 29 (promo rank)
  Position pos = make_pos(sq_bb(25), 0, 0, 0, BLACK);
  Move m{25, 29, 0, true};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN],  sq_bb(25));
  EXPECT_EQ(pos.bb[BLACK][KING], Bitboard{0});
}

TEST_F(UndoMoveTest, MultipleCapturedMen_AllRestored) {
  // Black man sq 1 captures sq 5 and sq 14, lands on sq 19
  Position pos = make_pos(sq_bb(1), 0, sq_bb(5) | sq_bb(14), 0, BLACK);
  Move m{1, 19, sq_bb(5) | sq_bb(14), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.bb[BLACK][MAN], sq_bb(1));
  EXPECT_EQ(pos.bb[WHITE][MAN], sq_bb(5) | sq_bb(14));
}

// ---- occupied ---------------------------------------------------------------

TEST_F(UndoMoveTest, Occupied_Restored) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  Bitboard original_occupied = pos.occupied;
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.occupied, original_occupied);
}

// ---- side to move -----------------------------------------------------------

TEST_F(UndoMoveTest, SideToMove_Restored) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.side_to_move, BLACK);
}

// ---- hash -------------------------------------------------------------------

TEST_F(UndoMoveTest, Hash_RestoredAfterQuietMove) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK);
  uint64_t original_hash = pos.hash;
  Move m{9, 13, 0, false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.hash, original_hash);
}

TEST_F(UndoMoveTest, Hash_RestoredAfterCapture) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK);
  uint64_t original_hash = pos.hash;
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.hash, original_hash);
}

TEST_F(UndoMoveTest, Hash_RestoredAfterPromotion) {
  Position pos = make_pos(sq_bb(25), 0, 0, 0, BLACK);
  uint64_t original_hash = pos.hash;
  Move m{25, 29, 0, true};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.hash, original_hash);
}

// ---- counters ---------------------------------------------------------------

TEST_F(UndoMoveTest, Ply_Restored) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK, 3);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.ply, uint16_t{3});
}

TEST_F(UndoMoveTest, Reversible_RestoredAfterQuietMove) {
  Position pos = make_pos(sq_bb(9), 0, 0, 0, BLACK, 0, 5);
  Move m{9, 13, 0, false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.reversible, uint8_t{5});
}

TEST_F(UndoMoveTest, Reversible_RestoredAfterCapture) {
  Position pos = make_pos(sq_bb(5), 0, sq_bb(9), 0, BLACK, 0, 7);
  Move m{5, 12, sq_bb(9), false};
  pos.do_move(m);
  pos.undo_move(m);
  EXPECT_EQ(pos.reversible, uint8_t{7});
}
