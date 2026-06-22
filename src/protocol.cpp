#include "draughts/protocol.h"

#include <cctype>
#include <sstream>
#include <string>

#include "draughts/bitboard.h"
#include "draughts/eval.h"
#include "draughts/notation.h"
#include "draughts/perft.h"
#include "draughts/search.h"

namespace draughts {

namespace {

std::string first_word_lower(const std::string& line) {
  size_t i = 0;
  while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
  size_t j = i;
  while (j < line.size() && !std::isspace((unsigned char)line[j])) ++j;
  std::string w = line.substr(i, j - i);
  for (char& c : w) c = (char)std::tolower((unsigned char)c);
  return w;
}

void print_board(const Position& pos, std::ostream& out) {
  char board[8][8];
  for (auto& row : board)
    for (char& cell : row) cell = '.';

  auto place = [&](Bitboard bb, char sym) {
    while (bb) {
      Square s = pop_lsb(bb);
      int sq64 = Board32to64[s];
      board[sq64 / 8][sq64 % 8] = sym;
    }
  };

  place(pos.bb[BLACK][MAN], 'b');
  place(pos.bb[BLACK][KING], 'B');
  place(pos.bb[WHITE][MAN], 'w');
  place(pos.bb[WHITE][KING], 'W');

  out << "+---+---+---+---+---+---+---+---+\n";
  for (int r = 0; r < 8; ++r) {
    out << '|';
    for (int c = 0; c < 8; ++c) out << ' ' << board[r][c] << " |";
    out << '\n';
    out << "+---+---+---+---+---+---+---+---+\n";
  }
  out << "Side to move: " << (pos.side_to_move == BLACK ? "Black" : "White")
      << '\n';
  out << to_pdn_position(pos) << '\n';
}

}  // namespace

TextProtocol::TextProtocol(Evaluator& e) : pos(Position::start_position()) {
  eval = &e;
}

void TextProtocol::run(std::istream& in, std::ostream& out) {
  std::string line;
  while (std::getline(in, line)) {
    handle_command(line, out);
    std::string cmd = first_word_lower(line);
    if (cmd == "quit" || cmd == "exit") break;
  }
}

void TextProtocol::handle_command(const std::string& line, std::ostream& out) {
  size_t i = 0;
  while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
  if (i == line.size()) return;

  size_t j = i;
  while (j < line.size() && !std::isspace((unsigned char)line[j])) ++j;

  std::string cmd = line.substr(i, j - i);
  for (char& c : cmd) c = (char)std::tolower((unsigned char)c);

  while (j < line.size() && std::isspace((unsigned char)line[j])) ++j;
  std::string rest = line.substr(j);

  if (cmd == "quit" || cmd == "exit") {
    return;
  } else if (cmd == "position") {
    pos = parse_pdn_position(rest);
  } else if (cmd == "move") {
    Move m = parse_move(rest, pos);
    if (is_null(m))
      out << "error: illegal move\n";
    else
      pos.do_move(m);
  } else if (cmd == "go") {
    SearchParams p = params;
    std::istringstream ss(rest);
    std::string tok;
    while (ss >> tok) {
      for (char& c : tok) c = (char)std::tolower((unsigned char)c);
      if (tok == "depth") {
        int d;
        if (ss >> d) p.max_depth = d;
      } else if (tok == "time") {
        int t;
        if (ss >> t) p.time_ms = t;
      }
    }
    SearchResult r = search(pos, p, *eval, [&](const SearchResult& ri) {
      out << "info depth " << ri.depth << " score " << ri.score << " nodes "
          << ri.nodes << " time " << ri.elapsed_ms << " move "
          << move_to_string(ri.best_move) << '\n';
    });
    out << "bestmove "
        << (is_null(r.best_move) ? "none" : move_to_string(r.best_move))
        << '\n';
  } else if (cmd == "perft") {
    int depth = 0;
    try {
      depth = std::stoi(rest);
    } catch (...) {
    }
    if (depth < 1)
      out << "error: usage: perft <depth>\n";
    else
      perft_divide(pos, depth, out);
  } else if (cmd == "print") {
    print_board(pos, out);
  } else {
    out << "error: unknown command '" << cmd << "'\n";
  }
}

}  // namespace draughts
