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

// ============================================================================
// rebuild_derived
// ============================================================================

class RebuildDerivedTest : public ::testing::Test {
 protected:
  // Build a position with bb[] set correctly but occupied/empty_sq left dirty.
  static Position make_dirty(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                             Bitboard w_kings) {
    Position pos{};
    pos.bb[BLACK][MAN]  = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN]  = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.occupied        = ALL_SQUARES;  // deliberately wrong
    pos.empty_sq        = 0;            // deliberately wrong
    return pos;
  }
};

// ---- occupied ---------------------------------------------------------------

TEST_F(RebuildDerivedTest, EmptyBoard_OccupiedIsZero) {
  Position pos = make_dirty(0, 0, 0, 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, Bitboard{0});
}

TEST_F(RebuildDerivedTest, SingleBlackMan_OccupiedMatchesBitboard) {
  Position pos = make_dirty(sq_bb(9), 0, 0, 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, sq_bb(9));
}

TEST_F(RebuildDerivedTest, SingleWhiteMan_OccupiedMatchesBitboard) {
  Position pos = make_dirty(0, 0, sq_bb(21), 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, sq_bb(21));
}

TEST_F(RebuildDerivedTest, SingleBlackKing_OccupiedMatchesBitboard) {
  Position pos = make_dirty(0, sq_bb(15), 0, 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, sq_bb(15));
}

TEST_F(RebuildDerivedTest, SingleWhiteKing_OccupiedMatchesBitboard) {
  Position pos = make_dirty(0, 0, 0, sq_bb(28));
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, sq_bb(28));
}

TEST_F(RebuildDerivedTest, AllFourPieceTypes_OccupiedIsUnion) {
  // One piece of each kind, all on distinct squares.
  Bitboard b_men  = sq_bb(1);
  Bitboard b_kings = sq_bb(3);
  Bitboard w_men  = sq_bb(28);
  Bitboard w_kings = sq_bb(30);
  Position pos = make_dirty(b_men, b_kings, w_men, w_kings);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, b_men | b_kings | w_men | w_kings);
}

TEST_F(RebuildDerivedTest, StartLayout_OccupiedMatchesDocumentation) {
  // Per bitboard.h: BLACK men on bits 0-11, WHITE men on bits 20-31.
  Bitboard b_men  = 0x00000FFFu;
  Bitboard w_men  = 0xFFF00000u;
  Position pos = make_dirty(b_men, 0, w_men, 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, 0xFFF00FFFu);
}

TEST_F(RebuildDerivedTest, DirtyOccupied_IsOverwritten) {
  Position pos = make_dirty(sq_bb(5), 0, sq_bb(17), 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied, sq_bb(5) | sq_bb(17));
}

// ---- empty_sq ---------------------------------------------------------------

TEST_F(RebuildDerivedTest, EmptyBoard_EmptySqIsAllSquares) {
  Position pos = make_dirty(0, 0, 0, 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.empty_sq, ALL_SQUARES);
}

TEST_F(RebuildDerivedTest, EmptySqIsComplementOfOccupied) {
  Bitboard b_men  = sq_bb(0) | sq_bb(9);
  Bitboard w_kings = sq_bb(25) | sq_bb(31);
  Position pos = make_dirty(b_men, 0, 0, w_kings);
  pos.rebuild_derived();
  EXPECT_EQ(pos.empty_sq, ~pos.occupied);
}

TEST_F(RebuildDerivedTest, DirtyEmptySq_IsOverwritten) {
  Position pos = make_dirty(sq_bb(7), 0, sq_bb(24), 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.empty_sq, ~(sq_bb(7) | sq_bb(24)));
}

TEST_F(RebuildDerivedTest, OccupiedAndEmptySqAreDisjointAndCoverAllSquares) {
  Position pos = make_dirty(sq_bb(1) | sq_bb(2), 0, sq_bb(29) | sq_bb(30), 0);
  pos.rebuild_derived();
  EXPECT_EQ(pos.occupied & pos.empty_sq, Bitboard{0});
  EXPECT_EQ(pos.occupied | pos.empty_sq, ALL_SQUARES);
}

// ============================================================================
// start_position
// ============================================================================

class StartPositionTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }
};

// ---- piece bitboards --------------------------------------------------------

TEST_F(StartPositionTest, BlackMenOnRows0To2) {
  EXPECT_EQ(Position::start_position().bb[BLACK][MAN], Bitboard{0x00000FFF});
}

TEST_F(StartPositionTest, NoBlackKingsAtStart) {
  EXPECT_EQ(Position::start_position().bb[BLACK][KING], Bitboard{0});
}

TEST_F(StartPositionTest, WhiteMenOnRows5To7) {
  EXPECT_EQ(Position::start_position().bb[WHITE][MAN], Bitboard{0xFFF00000});
}

TEST_F(StartPositionTest, NoWhiteKingsAtStart) {
  EXPECT_EQ(Position::start_position().bb[WHITE][KING], Bitboard{0});
}

// ---- derived fields ---------------------------------------------------------

TEST_F(StartPositionTest, OccupiedIs24Squares) {
  EXPECT_EQ(Position::start_position().occupied, Bitboard{0xFFF00FFF});
}

TEST_F(StartPositionTest, EmptySqIs8MiddleSquares) {
  EXPECT_EQ(Position::start_position().empty_sq, Bitboard{0x000FF000});
}

TEST_F(StartPositionTest, OccupiedAndEmptySqCoverAllSquares) {
  Position pos = Position::start_position();
  EXPECT_EQ(pos.occupied | pos.empty_sq, ALL_SQUARES);
  EXPECT_EQ(pos.occupied & pos.empty_sq, Bitboard{0});
}

// ---- piece counts -----------------------------------------------------------

TEST_F(StartPositionTest, TwelveBlackPieces) {
  EXPECT_EQ(Position::start_position().count(BLACK), 12);
}

TEST_F(StartPositionTest, TwelveWhitePieces) {
  EXPECT_EQ(Position::start_position().count(WHITE), 12);
}

TEST_F(StartPositionTest, TwentyFourPiecesTotal) {
  EXPECT_EQ(Position::start_position().total(), 24);
}

// ---- game state -------------------------------------------------------------

TEST_F(StartPositionTest, BlackMovesFirst) {
  EXPECT_EQ(Position::start_position().side_to_move, BLACK);
}

TEST_F(StartPositionTest, PlyIsZero) {
  EXPECT_EQ(Position::start_position().ply, uint16_t{0});
}

TEST_F(StartPositionTest, ReversibleIsZero) {
  EXPECT_EQ(Position::start_position().reversible, uint8_t{0});
}

// ---- hash -------------------------------------------------------------------

TEST_F(StartPositionTest, HashIsConsistentWithComputeHash) {
  Position pos = Position::start_position();
  EXPECT_EQ(pos.hash, compute_hash(pos));
}

TEST_F(StartPositionTest, TwoCallsProduceSameHash) {
  EXPECT_EQ(Position::start_position().hash, Position::start_position().hash);
}

// ============================================================================
// empty_position
// ============================================================================

TEST(EmptyPositionTest, AllBitboardsAreZero) {
  Position pos = Position::empty_position();
  EXPECT_EQ(pos.bb[BLACK][MAN],  Bitboard{0});
  EXPECT_EQ(pos.bb[BLACK][KING], Bitboard{0});
  EXPECT_EQ(pos.bb[WHITE][MAN],  Bitboard{0});
  EXPECT_EQ(pos.bb[WHITE][KING], Bitboard{0});
}

TEST(EmptyPositionTest, OccupiedIsZero) {
  EXPECT_EQ(Position::empty_position().occupied, Bitboard{0});
}

TEST(EmptyPositionTest, EmptySqIsAllSquares) {
  EXPECT_EQ(Position::empty_position().empty_sq, ALL_SQUARES);
}

TEST(EmptyPositionTest, TotalPieceCountIsZero) {
  EXPECT_EQ(Position::empty_position().total(), 0);
}

TEST(EmptyPositionTest, BlackMovesFirst) {
  EXPECT_EQ(Position::empty_position().side_to_move, BLACK);
}

TEST(EmptyPositionTest, PlyIsZero) {
  EXPECT_EQ(Position::empty_position().ply, uint16_t{0});
}

TEST(EmptyPositionTest, ReversibleIsZero) {
  EXPECT_EQ(Position::empty_position().reversible, uint8_t{0});
}

TEST(EmptyPositionTest, HashIsZero) {
  // No pieces, BLACK to move: no Zobrist keys are XOR'd, so hash == 0.
  EXPECT_EQ(Position::empty_position().hash, uint64_t{0});
}
