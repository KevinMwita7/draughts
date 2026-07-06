#pragma once
#include <atomic>
#include <iosfwd>
#include <mutex>
#include <string>
#include <thread>

#include "position.h"
#include "search.h"

namespace draughts {

struct Evaluator;

struct Protocol {
  virtual ~Protocol() = default;
  // Block until the session ends.  Reads from 'in', writes to 'out'.
  virtual void run(std::istream& in, std::ostream& out) = 0;
};

// ---- Simple line-oriented text protocol -------------------------------------
// Commands (case-insensitive):
//   ucinewgame              — reset to start position
//   isready                 — respond readyok
//   setoption name <n> ...  — parsed and ignored
//   position <pdn>          — set board from PDN position string
//   move <move>             — make a move (PDN notation)
//   moves                   — list all legal moves for the side to move
//   go [depth N] [movetime N] [ponder] — search and print best move
//   stop                    — stop searching
//   ponderhit               — switch from ponder to real search
//   perft <depth>           — run perft and print node count
//   d                       — display current position
//   quit / exit             — end session
//
// Responses are written as plain text lines to 'out'.
/*https :  // wbec-ridderkerk.nl/html/UCIProtocol.html*/
struct TextProtocol : Protocol {
  Position pos;
  SearchParams params;
  Evaluator* eval;  // non-owning; must outlive TextProtocol

  explicit TextProtocol(Evaluator& e);
  ~TextProtocol();
  void run(std::istream& in, std::ostream& out) override;
  void handle_command(const std::string& line, std::ostream& out);

 private:
  void stop_search();  // set flag + join if joinable

  std::atomic<bool> stop_flag_{false};
  std::thread search_thread_;
  std::mutex out_mutex_;
  SearchParams pending_go_params_{};
  bool pondering_{false};
  std::atomic<int> ponder_switch_ms_{0};
};
}  // namespace draughts
