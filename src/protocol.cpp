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

TextProtocol::~TextProtocol() { stop_search(); }

void TextProtocol::stop_search() {
  stop_flag_.store(true, std::memory_order_relaxed);
  if (search_thread_.joinable()) search_thread_.join();
  stop_flag_.store(false, std::memory_order_relaxed);
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
  std::string cmd = first_word_lower(line);
  if (cmd.empty()) return;

  size_t off = 0;
  while (off < line.size() && std::isspace((unsigned char)line[off])) ++off;
  off += cmd.size();
  while (off < line.size() && std::isspace((unsigned char)line[off])) ++off;
  std::string rest = line.substr(off);

  if (cmd == "quit" || cmd == "exit") {
    stop_search();
    return;
  } else if (cmd == "ucinewgame") {
    stop_search();
    pos = Position::start_position();
    params = SearchParams{};
  } else if (cmd == "isready") {
    std::lock_guard<std::mutex> lk(out_mutex_);
    out << "readyok\n";
  } else if (cmd == "setoption") {
    // parsed and ignored
  } else if (cmd == "position") {
    pos = parse_pdn_position(rest);
  } else if (cmd == "move") {
    Move m = parse_move(rest, pos);
    if (is_null(m))
      out << "error: illegal move\n";
    else
      pos.do_move(m);
  } else if (cmd == "stop") {
    stop_search();
  } else if (cmd == "ponderhit") {
    // Don't stop the ponder search — it's already on the right position.
    // Signal it to switch to real time controls.
    pondering_ = false;
    int real_time = pending_go_params_.time_ms;
    if (real_time > 0)
      ponder_switch_ms_.store(real_time, std::memory_order_release);
    // If real_time == 0 (no movetime given), search runs until 'stop'.
  } else if (cmd == "go") {
    stop_search();
    SearchParams p = params;
    p.stop_signal = &stop_flag_;
    bool ponder = false;
    std::istringstream ss(rest);
    std::string tok;
    while (ss >> tok) {
      for (char& c : tok) c = (char)std::tolower((unsigned char)c);
      if (tok == "depth") {
        int d;
        if (ss >> d) p.max_depth = d;
      } else if (tok == "movetime") {
        int t;
        if (ss >> t) p.time_ms = t;
      } else if (tok == "ponder") {
        ponder = true;
      }
    }
    // Save real params BEFORE the ponder override so ponderhit gets the right
    // time.
    pending_go_params_ = p;
    pending_go_params_.stop_signal = nullptr;
    pending_go_params_.switch_time_ms = nullptr;
    pondering_ = ponder;
    if (ponder) {
      ponder_switch_ms_.store(0, std::memory_order_relaxed);
      p.time_ms = 0;
      p.max_depth = 64;
      p.switch_time_ms = &ponder_switch_ms_;
    }
    Position pos_copy = pos;
    search_thread_ = std::thread([this, pos_copy, p, &out]() mutable {
      SearchResult r = search(pos_copy, p, *eval, [&](const SearchResult& ri) {
        std::lock_guard<std::mutex> lk(out_mutex_);
        out << "info depth " << ri.depth << " score " << ri.score << " nodes "
            << ri.nodes << " time " << ri.elapsed_ms << " move "
            << move_to_string(ri.best_move) << '\n';
      });
      std::lock_guard<std::mutex> lk(out_mutex_);
      out << "bestmove "
          << (is_null(r.best_move) ? "none" : move_to_string(r.best_move))
          << '\n';
    });
    // Ponder searches run until ponderhit/stop; all other go commands must
    // complete before handle_command returns so callers see the full output and
    // the captured &out reference stays valid.
    if (!ponder && search_thread_.joinable()) search_thread_.join();
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
  } else if (cmd == "d") {
    print_board(pos, out);
  } else {
    out << "error: unknown command '" << cmd << "'\n";
  }
}

}  // namespace draughts
