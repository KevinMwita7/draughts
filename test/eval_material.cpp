#include <gtest/gtest.h>

#include "draughts/bitboard.h"
#include "draughts/eval_material.h"
#include "draughts/position.h"

using namespace draughts;

// Formula (from side_to_move's perspective):
//   man_value * (own_men - opp_men) + king_value * (own_kings - opp_kings)

static Position make_pos(Bitboard b_men, Bitboard b_kings,
                         Bitboard w_men,  Bitboard w_kings,
                         Color side) {
    Position pos{};
    pos.bb[BLACK][MAN]  = b_men;
    pos.bb[BLACK][KING] = b_kings;
    pos.bb[WHITE][MAN]  = w_men;
    pos.bb[WHITE][KING] = w_kings;
    pos.occupied     = b_men | b_kings | w_men | w_kings;
    pos.empty_sq     = ~pos.occupied;
    pos.side_to_move = side;
    return pos;
}

// ---- default values ---------------------------------------------------------

TEST(MaterialEvalTest, DefaultManValueIs100) {
    EXPECT_EQ(MaterialEval{}.man_value, 100);
}

TEST(MaterialEvalTest, DefaultKingValueIs150) {
    EXPECT_EQ(MaterialEval{}.king_value, 150);
}

// ---- equal material ---------------------------------------------------------

TEST(MaterialEvalTest, EqualMenScoresZero) {
    MaterialEval ev;
    Position pos = make_pos(sq_bb(1), 0, sq_bb(20), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 0);
}

TEST(MaterialEvalTest, EqualKingsScoresZero) {
    MaterialEval ev;
    Position pos = make_pos(0, sq_bb(13), 0, sq_bb(17), BLACK);
    EXPECT_EQ(ev.evaluate(pos), 0);
}

TEST(MaterialEvalTest, EqualMenAndKingsScoresZero) {
    MaterialEval ev;
    Position pos = make_pos(sq_bb(1), sq_bb(13), sq_bb(20), sq_bb(17), BLACK);
    EXPECT_EQ(ev.evaluate(pos), 0);
}

TEST(MaterialEvalTest, EmptyBoardScoresZero) {
    MaterialEval ev;
    Position pos = make_pos(0, 0, 0, 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 0);
}

TEST(MaterialEvalTest, StartPositionScoresZero) {
    MaterialEval ev;
    EXPECT_EQ(ev.evaluate(Position::start_position()), 0);
}

// ---- man difference ---------------------------------------------------------

TEST(MaterialEvalTest, OneExtraOwnManReturnsManValue) {
    MaterialEval ev;
    // 2 black men vs 1 white man, BLACK to move → 100 * (2 - 1) = 100
    Position pos = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 100);
}

TEST(MaterialEvalTest, OneExtraOppManReturnsNegativeManValue) {
    MaterialEval ev;
    // 1 black man vs 2 white men, BLACK to move → 100 * (1 - 2) = -100
    Position pos = make_pos(sq_bb(1), 0, sq_bb(20) | sq_bb(21), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), -100);
}

TEST(MaterialEvalTest, LoneOwnManOnlyPosition) {
    MaterialEval ev;
    // BLACK has 1 man, WHITE has 0
    Position pos = make_pos(sq_bb(1), 0, 0, 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 100);
}

// ---- king difference --------------------------------------------------------

TEST(MaterialEvalTest, OneExtraOwnKingReturnsKingValue) {
    MaterialEval ev;
    // 1 black king, no white kings, BLACK to move → 150
    Position pos = make_pos(0, sq_bb(15), 0, 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 150);
}

TEST(MaterialEvalTest, OneExtraOppKingReturnsNegativeKingValue) {
    MaterialEval ev;
    Position pos = make_pos(0, 0, 0, sq_bb(15), BLACK);
    EXPECT_EQ(ev.evaluate(pos), -150);
}

// ---- mixed material ---------------------------------------------------------

TEST(MaterialEvalTest, MixedMaterialSumsTermsCorrectly) {
    MaterialEval ev;
    // 2 black men + 1 black king vs 1 white man, BLACK to move
    // = 100*(2-1) + 150*(1-0) = 250
    Position pos = make_pos(sq_bb(1) | sq_bb(2), sq_bb(15), sq_bb(20), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 250);
}

TEST(MaterialEvalTest, ManAndKingTermsCanCancel) {
    MaterialEval ev;
    // 1 black man + 1 white king, BLACK to move
    // = 100*(1-0) + 150*(0-1) = 100 - 150 = -50
    Position pos = make_pos(sq_bb(1), 0, 0, sq_bb(28), BLACK);
    EXPECT_EQ(ev.evaluate(pos), -50);
}

// ---- perspective (side to move) ---------------------------------------------

TEST(MaterialEvalTest, PerspectiveFlipsSignForWhite) {
    MaterialEval ev;
    // 2 black men vs 1 white man — good for BLACK, bad for WHITE
    Position black_stm = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, BLACK);
    Position white_stm = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, WHITE);
    EXPECT_EQ(ev.evaluate(black_stm),  100);
    EXPECT_EQ(ev.evaluate(white_stm), -100);
}

TEST(MaterialEvalTest, MixedMaterialWhitePerspective) {
    MaterialEval ev;
    // Same mixed position as above but WHITE to move → -250
    Position pos = make_pos(sq_bb(1) | sq_bb(2), sq_bb(15), sq_bb(20), 0, WHITE);
    EXPECT_EQ(ev.evaluate(pos), -250);
}

// ---- custom weights ---------------------------------------------------------

TEST(MaterialEvalTest, CustomManValueScalesResult) {
    MaterialEval ev;
    ev.man_value = 200;
    Position pos = make_pos(sq_bb(1) | sq_bb(2), 0, sq_bb(20), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 200);  // 200 * (2-1)
}

TEST(MaterialEvalTest, CustomKingValueScalesResult) {
    MaterialEval ev;
    ev.king_value = 300;
    Position pos = make_pos(0, sq_bb(13), 0, 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 300);
}

TEST(MaterialEvalTest, CustomWeightsAreBothUsed) {
    MaterialEval ev;
    ev.man_value  = 50;
    ev.king_value = 200;
    // 1 extra own man + 1 extra own king = 50 + 200 = 250
    Position pos = make_pos(sq_bb(1) | sq_bb(2), sq_bb(15), sq_bb(20), 0, BLACK);
    EXPECT_EQ(ev.evaluate(pos), 250);
}
