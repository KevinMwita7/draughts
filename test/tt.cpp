#include <gtest/gtest.h>
#include "draughts/tt.h"
#include "draughts/types.h"

using namespace draughts;

// Arbitrary hashes that map to the same slot in a 1024-entry table.
// slot = hash & 1023; same low 10 bits, different upper 32 bits.
static constexpr uint64_t HASH_A = 0xDEADBEEF00000007ULL;
static constexpr uint64_t HASH_B = 0xCAFEBABE00000007ULL; // same slot as A
static constexpr uint64_t HASH_C = 0xDEADBEEF00000042ULL; // different slot

static constexpr Move MOVE_05_12 = {5, 12, 0, false};
static constexpr Move MOVE_09_13 = {9, 13, 0, false};

// ============================================================================
// Construction / empty table
// ============================================================================

TEST(TTTest, FreshTable_ProbeReturnsFalse) {
    TranspositionTable tt(1024);
    TTEntry e;
    EXPECT_FALSE(tt.probe(HASH_A, &e));
}

TEST(TTTest, FreshTable_HashfullIsZero) {
    TranspositionTable tt(1024);
    EXPECT_EQ(tt.hashfull(), 0);
}

// ============================================================================
// Store → probe round-trip
// ============================================================================

TEST(TTTest, StoreExact_ProbeReturnsAllFields) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 5, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score,     int16_t{100});
    EXPECT_EQ(e.move_from, uint8_t{5});
    EXPECT_EQ(e.move_to,   uint8_t{12});
    EXPECT_EQ(e.depth,     uint8_t{5});
    EXPECT_EQ(e.flag,      TTFlag::EXACT);
}

TEST(TTTest, StoreLowerBound_FlagRoundTrips) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{200}, MOVE_05_12, 3, TTFlag::LOWER_BOUND);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.flag, TTFlag::LOWER_BOUND);
}

TEST(TTTest, StoreUpperBound_FlagRoundTrips) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{-50}, MOVE_05_12, 4, TTFlag::UPPER_BOUND);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.flag, TTFlag::UPPER_BOUND);
}

TEST(TTTest, StoreNullMove_MoveFromIsSqNone) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{0}, NULL_MOVE, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.move_from, SQ_NONE);
}

// ============================================================================
// Key collision guard
// ============================================================================

TEST(TTTest, DifferentUpperKey_SameSlot_ProbeReturnsFalse) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{42}, MOVE_05_12, 5, TTFlag::EXACT);

    // HASH_B maps to the same slot but has a different upper-32 key.
    TTEntry e;
    EXPECT_FALSE(tt.probe(HASH_B, &e));
}

TEST(TTTest, DifferentSlot_ProbeReturnsFalse) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{42}, MOVE_05_12, 5, TTFlag::EXACT);
    TTEntry e;
    EXPECT_FALSE(tt.probe(HASH_C, &e));
}

// ============================================================================
// Replacement policy
// ============================================================================

TEST(TTTest, SameHash_ShallowerIncoming_DeepEntryKept) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 5, TTFlag::EXACT);
    tt.store(HASH_A, Score{999}, MOVE_09_13, 3, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score,     int16_t{100});
    EXPECT_EQ(e.move_from, uint8_t{5});
    EXPECT_EQ(e.depth,     uint8_t{5});
}

TEST(TTTest, SameHash_DeeperIncoming_ReplacesEntry) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 3, TTFlag::EXACT);
    tt.store(HASH_A, Score{999}, MOVE_09_13, 5, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score,     int16_t{999});
    EXPECT_EQ(e.move_from, uint8_t{9});
    EXPECT_EQ(e.depth,     uint8_t{5});
}

TEST(TTTest, SameHash_EqualDepth_ReplacesEntry) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 4, TTFlag::EXACT);
    tt.store(HASH_A, Score{999}, MOVE_09_13, 4, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score, int16_t{999});
}

TEST(TTTest, DifferentHash_SameSlot_AlwaysReplaces) {
    TranspositionTable tt(1024);
    // Store deep entry for HASH_A.
    tt.store(HASH_A, Score{100}, MOVE_05_12, 9, TTFlag::EXACT);
    // HASH_B maps to the same slot — different position, always replaces.
    tt.store(HASH_B, Score{777}, MOVE_09_13, 1, TTFlag::LOWER_BOUND);

    TTEntry e;
    // HASH_A is gone; HASH_B is present.
    EXPECT_FALSE(tt.probe(HASH_A, &e));
    ASSERT_TRUE(tt.probe(HASH_B, &e));
    EXPECT_EQ(e.score, int16_t{777});
}

// ============================================================================
// Clear
// ============================================================================

TEST(TTTest, Clear_PreviousEntryNoLongerFound) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{42}, MOVE_05_12, 5, TTFlag::EXACT);
    tt.clear();
    TTEntry e;
    EXPECT_FALSE(tt.probe(HASH_A, &e));
}

TEST(TTTest, Clear_HashfullDropsToZero) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{42}, MOVE_05_12, 5, TTFlag::EXACT);
    tt.clear();
    EXPECT_EQ(tt.hashfull(), 0);
}

// ============================================================================
// Hashfull
// ============================================================================

TEST(TTTest, AllSlotsOccupied_HashfullIs1000) {
    // 8-entry table so we can fill every slot easily.
    TranspositionTable tt(8);
    for (uint64_t i = 0; i < 8; ++i) {
        // Construct a hash whose low 3 bits == i (slot i), arbitrary upper bits.
        uint64_t h = (static_cast<uint64_t>(i + 1) << 32) | i;
        tt.store(h, Score{0}, NULL_MOVE, 1, TTFlag::EXACT);
    }
    EXPECT_EQ(tt.hashfull(), 1000);
}

// ============================================================================
// resize — power-of-two rounding and cap
// ============================================================================

TEST(TTTest, Resize_NonPowerOfTwo_RoundsUpToNextPow2) {
    // 100 entries → rounds up to 128; slot = hash & 127
    TranspositionTable tt(100);
    // hash with low 7 bits = 0x7F (slot 127, valid only if size >= 128)
    constexpr uint64_t h = 0xABCD000000007FULL;
    tt.store(h, Score{1}, NULL_MOVE, 1, TTFlag::EXACT);
    TTEntry e;
    EXPECT_TRUE(tt.probe(h, &e));
}

// ============================================================================
// Generation / eviction
// ============================================================================

TEST(TTTest, OldGenEntry_AlwaysReplaced) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 9, TTFlag::EXACT);
    tt.new_search();
    // Shallower entry from a new generation should replace the deep old one.
    tt.store(HASH_A, Score{42}, MOVE_09_13, 1, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score,     int16_t{42});
    EXPECT_EQ(e.move_from, uint8_t{9});
    EXPECT_EQ(e.depth,     uint8_t{1});
}

TEST(TTTest, CurrentGenEntry_DeeperProtected) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, Score{100}, MOVE_05_12, 9, TTFlag::EXACT);
    // No new_search() — same generation, shallower incoming should be rejected.
    tt.store(HASH_A, Score{42}, MOVE_09_13, 1, TTFlag::EXACT);

    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.score,     int16_t{100});
    EXPECT_EQ(e.move_from, uint8_t{5});
    EXPECT_EQ(e.depth,     uint8_t{9});
}

// ============================================================================
// Score boundary values
// ============================================================================

TEST(TTTest, ScoreWin_RoundTrips) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, SCORE_WIN, NULL_MOVE, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(Score{e.score}, SCORE_WIN);
}

TEST(TTTest, ScoreLoss_RoundTrips) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, SCORE_LOSS, NULL_MOVE, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(Score{e.score}, SCORE_LOSS);
}

TEST(TTTest, ScoreDraw_RoundTrips) {
    TranspositionTable tt(1024);
    tt.store(HASH_A, SCORE_DRAW, NULL_MOVE, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(Score{e.score}, SCORE_DRAW);
}

// ============================================================================
// Move encoding
// ============================================================================

TEST(TTTest, MoveFromTo_BoundarySquares_RoundTrip) {
    TranspositionTable tt(1024);
    Move m{0, 31, 0, false};
    tt.store(HASH_A, Score{0}, m, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.move_from, uint8_t{0});
    EXPECT_EQ(e.move_to,   uint8_t{31});
}

TEST(TTTest, CaptureMove_OnlyFromToStored) {
    TranspositionTable tt(1024);
    // Captured bitboard and promotion are not stored in the TT.
    Move m{5, 12, /*captured=*/0xFFF, /*promotion=*/true};
    tt.store(HASH_A, Score{0}, m, 1, TTFlag::EXACT);
    TTEntry e;
    ASSERT_TRUE(tt.probe(HASH_A, &e));
    EXPECT_EQ(e.move_from, uint8_t{5});
    EXPECT_EQ(e.move_to,   uint8_t{12});
}
