#pragma once

#include "draughts/zobrist.h"

#include <unordered_set>

#include "draughts/bitboard.h"
#include "draughts/position.h"
#include "gtest/gtest.h"

using namespace draughts;

static Position make_empty_pos(Color side = BLACK) {
  Position pos{};
  pos.side_to_move = side;
  return pos;
}

static Position make_pos(Bitboard b_men, Bitboard b_kings, Bitboard w_men,
                         Bitboard w_kings, Color side) {
  Position pos{};
  pos.bb[BLACK][MAN] = b_men;
  pos.bb[BLACK][KING] = b_kings;
  pos.bb[WHITE][MAN] = w_men;
  pos.bb[WHITE][KING] = w_kings;
  pos.occupied = b_men | b_kings | w_men | w_kings;
  pos.side_to_move = side;
  return pos;
}

// ---- zobrist::init() ----------------------------------------------------

TEST(ZobristInit, SideKeyNonZeroAfterInit) {
  zobrist::init();
  EXPECT_NE(zobrist::SIDE_KEY, uint64_t{0});
}

TEST(ZobristInit, AllPieceKeysNonZeroAfterInit) {
  zobrist::init();
  for (int s = 0; s < 32; ++s)
    for (int p = 0; p < 4; ++p)
      EXPECT_NE(zobrist::PIECE_KEY[s][p], uint64_t{0})
          << "PIECE_KEY[" << s << "][" << p << "] is zero";
}

TEST(ZobristInit, AllPieceKeysDistinct) {
  zobrist::init();
  std::unordered_set<uint64_t> keys;
  for (int s = 0; s < 32; ++s)
    for (int p = 0; p < 4; ++p) keys.insert(zobrist::PIECE_KEY[s][p]);
  EXPECT_EQ(keys.size(), size_t{128});
}

TEST(ZobristInit, ReinitChangesKeys) {
  zobrist::init();
  uint64_t first = zobrist::SIDE_KEY;
  zobrist::init();
  EXPECT_NE(zobrist::SIDE_KEY, first);
}

// ---- zobrist::piece_key() -----------------------------------------------

TEST(ZobristPieceKey, MatchesPieceKeyArray) {
  zobrist::init();
  for (int s = 0; s < 32; ++s)
    for (int p = 0; p < 4; ++p)
      EXPECT_EQ(zobrist::piece_key(Square(s), Piece(p)),
                zobrist::PIECE_KEY[s][p]);
}

// ---- compute_hash() -----------------------------------------------------

TEST(ZobristComputeHash, EmptyBoardBlackToMoveIsZero) {
  zobrist::init();
  EXPECT_EQ(compute_hash(make_empty_pos(BLACK)), uint64_t{0});
}

TEST(ZobristComputeHash, EmptyBoardWhiteToMoveIsSideKey) {
  zobrist::init();
  EXPECT_EQ(compute_hash(make_empty_pos(WHITE)), zobrist::SIDE_KEY);
}

TEST(ZobristComputeHash, SideToMoveFlipChangesHash) {
  zobrist::init();
  EXPECT_NE(compute_hash(make_empty_pos(BLACK)),
            compute_hash(make_empty_pos(WHITE)));
}

TEST(ZobristComputeHash, SingleBlackManMatchesPieceKey) {
  zobrist::init();
  Position pos = make_pos(sq_bb(5), 0, 0, 0, BLACK);
  EXPECT_EQ(compute_hash(pos), zobrist::PIECE_KEY[5][B_MAN]);
}

TEST(ZobristComputeHash, SingleBlackKingMatchesPieceKey) {
  zobrist::init();
  Position pos = make_pos(0, sq_bb(15), 0, 0, BLACK);
  EXPECT_EQ(compute_hash(pos), zobrist::PIECE_KEY[15][B_KING]);
}

TEST(ZobristComputeHash, WhitePieceWithWhiteToMoveIncludesSideKey) {
  zobrist::init();
  Position pos = make_pos(0, 0, sq_bb(20), 0, WHITE);
  EXPECT_EQ(compute_hash(pos),
            zobrist::SIDE_KEY ^ zobrist::PIECE_KEY[20][W_MAN]);
}

TEST(ZobristComputeHash, MultiplePiecesHashIsXorOfPieceKeys) {
  zobrist::init();
  Position pos = make_pos(sq_bb(0) | sq_bb(4), 0, sq_bb(28), 0, BLACK);
  uint64_t expected = zobrist::PIECE_KEY[0][B_MAN] ^
                      zobrist::PIECE_KEY[4][B_MAN] ^
                      zobrist::PIECE_KEY[28][W_MAN];
  EXPECT_EQ(compute_hash(pos), expected);
}

TEST(ZobristComputeHash, SamePositionGivesSameHash) {
  zobrist::init();
  Position a = make_pos(sq_bb(3), 0, sq_bb(25), 0, BLACK);
  Position b = make_pos(sq_bb(3), 0, sq_bb(25), 0, BLACK);
  EXPECT_EQ(compute_hash(a), compute_hash(b));
}

TEST(ZobristComputeHash, IncrementalUpdateMatchesFullRecompute) {
  zobrist::init();

  // Full position: B_MAN on sq 3 and W_MAN on sq 20, BLACK to move.
  Position full = make_pos(sq_bb(3), 0, sq_bb(20), 0, BLACK);
  uint64_t full_hash = compute_hash(full);

  // Simulate the do_move incremental update: start from W_MAN on 20 only,
  // then XOR in the key for placing B_MAN on 3.
  Position prev = make_pos(0, 0, sq_bb(20), 0, BLACK);
  uint64_t incremental = compute_hash(prev) ^ zobrist::PIECE_KEY[3][B_MAN];

  EXPECT_EQ(incremental, full_hash);
}
