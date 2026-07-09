#include "draughts/protocol.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "draughts/bitboard.h"
#include "draughts/draw.h"
#include "draughts/eval_material.h"
#include "draughts/notation.h"
#include "draughts/position.h"
#include "draughts/zobrist.h"

using namespace draughts;

static std::string run_cmd(TextProtocol& p, const std::string& line) {
  std::ostringstream out;
  p.handle_command(line, out);
  return out.str();
}

static bool contains(const std::string& s, const std::string& needle) {
  return s.find(needle) != std::string::npos;
}

static int count_substr(const std::string& s, const std::string& needle) {
  int n = 0;
  size_t pos = 0;
  while ((pos = s.find(needle, pos)) != std::string::npos) {
    ++n;
    ++pos;
  }
  return n;
}

class TextProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override { zobrist::init(); }

  MaterialEval eval;
  TextProtocol proto{eval};
};

// ---- constructor
// ---------------------------------------------------------------

TEST_F(TextProtocolTest, ConstructorSetsStartPosition) {
  EXPECT_EQ(to_pdn_position(proto.pos),
            to_pdn_position(Position::start_position()));
}

TEST_F(TextProtocolTest, ConstructorSetsBlackToMove) {
  EXPECT_EQ(proto.pos.side_to_move, BLACK);
}

TEST_F(TextProtocolTest, ConstructorSetsDefaultParams) {
  EXPECT_EQ(proto.params.max_depth, 64);
  EXPECT_EQ(proto.params.time_ms, 0);
}

// ---- empty / whitespace
// --------------------------------------------------------

TEST_F(TextProtocolTest, EmptyLineProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, ""), "");
}

TEST_F(TextProtocolTest, WhitespaceOnlyLineProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "   \t  "), "");
}

// ---- unknown command
// -----------------------------------------------------------

TEST_F(TextProtocolTest, UnknownCommandReportsError) {
  std::string out = run_cmd(proto, "foobar");
  EXPECT_TRUE(contains(out, "error"));
  EXPECT_TRUE(contains(out, "foobar"));
}

// ---- quit / exit
// ---------------------------------------------------------------

TEST_F(TextProtocolTest, QuitProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "quit"), "");
}

TEST_F(TextProtocolTest, ExitProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "exit"), "");
}

TEST_F(TextProtocolTest, RunStopsOnQuit) {
  std::istringstream in("d\nquit\nd\n");
  std::ostringstream out;
  proto.run(in, out);
  // First d processed; second d (after quit) must not appear.
  EXPECT_EQ(count_substr(out.str(), "Side to move"), 1);
}

TEST_F(TextProtocolTest, RunStopsOnExit) {
  std::istringstream in("exit\n");
  std::ostringstream out;
  proto.run(in, out);
  EXPECT_EQ(out.str(), "");
}

TEST_F(TextProtocolTest, RunProcessesCommandsBeforeQuit) {
  std::istringstream in("move 9-13\nquit\n");
  std::ostringstream out;
  proto.run(in, out);
  EXPECT_EQ(proto.pos.side_to_move, WHITE);
}

// ---- case insensitivity
// --------------------------------------------------------

TEST_F(TextProtocolTest, CommandsAreCaseInsensitive) {
  EXPECT_TRUE(contains(run_cmd(proto, "D"), "Side to move"));
  EXPECT_TRUE(contains(run_cmd(proto, "d"), "Side to move"));
  EXPECT_EQ(run_cmd(proto, "QUIT"), "");
  EXPECT_EQ(run_cmd(proto, "EXIT"), "");
}

// ---- d command
// ------------------------------------------------------------------

TEST_F(TextProtocolTest, PrintShowsSideToMove) {
  EXPECT_TRUE(contains(run_cmd(proto, "d"), "Side to move: Black"));
}

TEST_F(TextProtocolTest, PrintShowsPDNString) {
  EXPECT_TRUE(contains(run_cmd(proto, "d"),
                       to_pdn_position(Position::start_position())));
}

TEST_F(TextProtocolTest, PrintShowsBoardBorders) {
  EXPECT_TRUE(contains(run_cmd(proto, "d"), "+---+"));
}

TEST_F(TextProtocolTest, PrintReflectsCurrentSideAfterPositionCommand) {
  run_cmd(proto, "position W:W21:B1");
  EXPECT_TRUE(contains(run_cmd(proto, "d"), "Side to move: White"));
}

// ---- position command
// ----------------------------------------------------------

TEST_F(TextProtocolTest, PositionCommandSetsPDN) {
  std::string pdn =
      "W:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12";
  run_cmd(proto, "position " + pdn);
  EXPECT_EQ(to_pdn_position(proto.pos), pdn);
}

TEST_F(TextProtocolTest, PositionCommandChangesSideToMove) {
  run_cmd(proto, "position W:W21:B1");
  EXPECT_EQ(proto.pos.side_to_move, WHITE);
}

TEST_F(TextProtocolTest, PositionCommandSetsKing) {
  run_cmd(proto, "position B:WK15:B3");
  EXPECT_EQ(to_pdn_position(proto.pos), "B:WK15:B3");
}

TEST_F(TextProtocolTest, PositionCommandOverwritesPreviousPosition) {
  run_cmd(proto, "position W:W21:B1");
  run_cmd(proto, "position B:WK15:B3");
  EXPECT_EQ(to_pdn_position(proto.pos), "B:WK15:B3");
}

// ---- move command
// --------------------------------------------------------------

// PDN square 9 = internal sq 8 (0-indexed); PDN 13 = internal 12.
TEST_F(TextProtocolTest, MoveCommandVacatesSourceSquare) {
  run_cmd(proto, "move 9-13");
  EXPECT_EQ(proto.pos.bb[BLACK][MAN] & sq_bb(8), 0u);
}

TEST_F(TextProtocolTest, MoveCommandFillsDestinationSquare) {
  run_cmd(proto, "move 9-13");
  EXPECT_NE(proto.pos.bb[BLACK][MAN] & sq_bb(12), 0u);
}

TEST_F(TextProtocolTest, MoveCommandFlipsSideToMove) {
  run_cmd(proto, "move 9-13");
  EXPECT_EQ(proto.pos.side_to_move, WHITE);
}

TEST_F(TextProtocolTest, IllegalMoveReportsError) {
  EXPECT_TRUE(contains(run_cmd(proto, "move 1-2"), "error"));
}

TEST_F(TextProtocolTest, IllegalMoveDoesNotChangePosition) {
  std::string before = to_pdn_position(proto.pos);
  run_cmd(proto, "move 1-2");
  EXPECT_EQ(to_pdn_position(proto.pos), before);
}

TEST_F(TextProtocolTest, TwoMovesAlternateSideToMove) {
  run_cmd(proto, "move 9-13");   // BLACK
  run_cmd(proto, "move 21-17");  // WHITE: PDN 21→17 = internal sq 20→16
  EXPECT_EQ(proto.pos.side_to_move, BLACK);
}

// ---- go command
// ----------------------------------------------------------------

TEST_F(TextProtocolTest, GoProducesBestmoveLine) {
  EXPECT_TRUE(contains(run_cmd(proto, "go depth 1"), "bestmove"));
}

TEST_F(TextProtocolTest, GoDepth1ProducesOneInfoLine) {
  EXPECT_EQ(count_substr(run_cmd(proto, "go depth 1"), "info depth"), 1);
}

TEST_F(TextProtocolTest, GoDepth2ProducesTwoInfoLines) {
  EXPECT_EQ(count_substr(run_cmd(proto, "go depth 2"), "info depth"), 2);
}

TEST_F(TextProtocolTest, GoInfoLineContainsScore) {
  EXPECT_TRUE(contains(run_cmd(proto, "go depth 1"), "score"));
}

TEST_F(TextProtocolTest, GoInfoLineContainsNodes) {
  EXPECT_TRUE(contains(run_cmd(proto, "go depth 1"), "nodes"));
}

TEST_F(TextProtocolTest, GoInfoLineContainsMove) {
  EXPECT_TRUE(contains(run_cmd(proto, "go depth 1"), "move"));
}

TEST_F(TextProtocolTest, GoDoesNotReturnBestmoveNoneFromStartPosition) {
  EXPECT_FALSE(contains(run_cmd(proto, "go depth 1"), "bestmove none"));
}

TEST_F(TextProtocolTest, GoReturnsNoneOnTerminalPosition) {
  // BLACK has no pieces → no legal moves.
  Position terminal{};
  terminal.bb[WHITE][MAN] = sq_bb(28);
  terminal.side_to_move = BLACK;
  terminal.rebuild_derived();
  proto.pos = terminal;

  EXPECT_TRUE(contains(run_cmd(proto, "go depth 1"), "bestmove none"));
}

TEST_F(TextProtocolTest, GoTimeFlagIsParsed) {
  // go time 10 should still complete without hanging (time_ms is a limit, not a
  // target, so a fast search at depth 1 finishes well within 10 ms).
  std::string out = run_cmd(proto, "go depth 1 movetime 10");
  EXPECT_TRUE(contains(out, "bestmove"));
}

// ---- perft command
// -------------------------------------------------------------

TEST_F(TextProtocolTest, PerftDepth1FromStartShowsTotal7) {
  EXPECT_TRUE(contains(run_cmd(proto, "perft 1"), "Total: 7"));
}

TEST_F(TextProtocolTest, PerftDepth1PrintsSevenMoveLines) {
  // Each of the 7 root moves at depth 1 contributes exactly 1 leaf node.
  EXPECT_EQ(count_substr(run_cmd(proto, "perft 1"), ": 1\n"), 7);
}

TEST_F(TextProtocolTest, PerftDepth2FromStartShowsTotal49) {
  EXPECT_TRUE(contains(run_cmd(proto, "perft 2"), "Total: 49"));
}

TEST_F(TextProtocolTest, PerftZeroDepthReportsError) {
  EXPECT_TRUE(contains(run_cmd(proto, "perft 0"), "error"));
}

TEST_F(TextProtocolTest, PerftNegativeDepthReportsError) {
  EXPECT_TRUE(contains(run_cmd(proto, "perft -1"), "error"));
}

TEST_F(TextProtocolTest, PerftNoArgReportsError) {
  EXPECT_TRUE(contains(run_cmd(proto, "perft"), "error"));
}

// ---- isready
// -----------------------------------------------------------

TEST_F(TextProtocolTest, IsReadyOutputsReadyOk) {
  EXPECT_EQ(run_cmd(proto, "isready"), "readyok\n");
}

TEST_F(TextProtocolTest, IsReadyCaseInsensitive) {
  EXPECT_EQ(run_cmd(proto, "ISREADY"), "readyok\n");
}

// ---- setoption
// ---------------------------------------------------------

TEST_F(TextProtocolTest, SetOptionProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "setoption name Hash value 16"), "");
}

TEST_F(TextProtocolTest, SetOptionDoesNotChangePosition) {
  std::string before = to_pdn_position(proto.pos);
  run_cmd(proto, "setoption name Hash value 16");
  EXPECT_EQ(to_pdn_position(proto.pos), before);
}

// ---- ucinewgame
// --------------------------------------------------------

TEST_F(TextProtocolTest, UciNewGameResetsToStartPosition) {
  run_cmd(proto, "move 9-13");
  run_cmd(proto, "ucinewgame");
  EXPECT_EQ(to_pdn_position(proto.pos),
            to_pdn_position(Position::start_position()));
}

TEST_F(TextProtocolTest, UciNewGameResetsSideToMove) {
  run_cmd(proto, "move 9-13");
  run_cmd(proto, "ucinewgame");
  EXPECT_EQ(proto.pos.side_to_move, BLACK);
}

TEST_F(TextProtocolTest, UciNewGameResetsSearchParams) {
  proto.params.max_depth = 5;
  run_cmd(proto, "ucinewgame");
  EXPECT_EQ(proto.params.max_depth, 64);
}

TEST_F(TextProtocolTest, UciNewGameProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "ucinewgame"), "");
}

// ---- stop
// --------------------------------------------------------------

TEST_F(TextProtocolTest, StopProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "stop"), "");
}

TEST_F(TextProtocolTest, StopWithNoActiveSearchProducesNoOutput) {
  // stop when idle must not crash or emit anything
  run_cmd(proto, "stop");
  EXPECT_EQ(run_cmd(proto, "stop"), "");
}

// ---- ponderhit
// ---------------------------------------------------------

TEST_F(TextProtocolTest, PonderHitWithNoActiveSearchProducesNoOutput) {
  EXPECT_EQ(run_cmd(proto, "ponderhit"), "");
}

TEST_F(TextProtocolTest, PonderHitThenStopEmitsBestmove) {
  // go ponder starts an unbounded search; ponderhit switches to real time;
  // stop terminates and joins the thread, which then writes bestmove.
  std::istringstream in("go ponder depth 64\nponderhit\nstop\n");
  std::ostringstream out;
  proto.run(in, out);
  EXPECT_TRUE(contains(out.str(), "bestmove"));
}

// ---- draw detection ------------------------------------------------------

// Black king shuffles 13<->18, White king shuffles 31<->27. Neither king can
// ever threaten a capture against the other (they stay rows apart), so this
// is a pure, endlessly repeatable cycle: after 4 plies the shuffle's start
// position recurs for the 2nd time, and after 8 plies for the 3rd.
static const char* kShuffleStart = "B:WK31:BK13";
static const char* kShuffleMoves[8] = {
    "13-18", "31-27", "18-13", "27-31",
    "13-18", "31-27", "18-13", "27-31",
};

TEST_F(TextProtocolTest, OrdinaryMoveDoesNotReportDraw) {
  EXPECT_FALSE(contains(run_cmd(proto, "move 9-13"), "draw"));
}

TEST_F(TextProtocolTest, CaptureDoesNotReportDraw) {
  run_cmd(proto, "position B:W13:B9");
  EXPECT_FALSE(contains(run_cmd(proto, "move 9x18"), "draw"));
}

TEST_F(TextProtocolTest, NoProgressRuleReportsDrawAtCutoff) {
  proto.pos.reversible = MAX_REVERSIBLE_PLIES - 1;
  EXPECT_TRUE(contains(run_cmd(proto, "move 9-13"), "draw"));
}

TEST_F(TextProtocolTest, RepetitionReportsDrawOnThirdOccurrenceOnly) {
  run_cmd(proto, std::string("position ") + kShuffleStart);
  for (int i = 0; i < 8; ++i) {
    std::string out = run_cmd(proto, std::string("move ") + kShuffleMoves[i]);
    if (i < 7)
      EXPECT_FALSE(contains(out, "draw")) << "premature draw at ply " << i + 1;
    else
      EXPECT_TRUE(contains(out, "draw"));
  }
}

TEST_F(TextProtocolTest, PositionCommandReplaysMovesAndDetectsRepetition) {
  std::string cmd_line = std::string("position ") + kShuffleStart + " moves";
  for (int i = 0; i < 8; ++i) cmd_line += std::string(" ") + kShuffleMoves[i];
  EXPECT_TRUE(contains(run_cmd(proto, cmd_line), "draw"));
}

TEST_F(TextProtocolTest, PositionCommandMovesReplayStopsAtBadToken) {
  // "1-2" isn't a legal move from the position reached after "13-18", so
  // replay must stop there rather than applying it or crashing.
  std::string cmd_line =
      std::string("position ") + kShuffleStart + " moves 13-18 1-2";
  run_cmd(proto, cmd_line);
  EXPECT_EQ(to_pdn_position(proto.pos), "W:WK31:BK18");
}

TEST_F(TextProtocolTest, PositionCommandWithoutMovesResetsHistory) {
  run_cmd(proto, std::string("position ") + kShuffleStart);
  for (int i = 0; i < 4; ++i)
    run_cmd(proto, std::string("move ") + kShuffleMoves[i]);
  // 4 plies back at the shuffle's starting position — a fresh 'position'
  // command must not remember that as a prior occurrence.
  run_cmd(proto, std::string("position ") + kShuffleStart);
  std::string out = run_cmd(proto, std::string("move ") + kShuffleMoves[0]);
  EXPECT_FALSE(contains(out, "draw"));
}
