#pragma once
#include <iosfwd>
#include <string>

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
//   position <pdn>          — set board from PDN position string
//   move <move>             — make a move (PDN notation)
//   go [depth N] [time N]   — search and print best move
//   perft <depth>           — run perft and print node count
//   print                   — display current position
//   quit / exit             — end session
//
// Responses are written as plain text lines to 'out'.

struct TextProtocol : Protocol {
  Position pos;
  SearchParams params;
  Evaluator* eval;  // non-owning; must outlive TextProtocol

  explicit TextProtocol(Evaluator& e);
  void run(std::istream& in, std::ostream& out) override;

 private:
  void handle_command(const std::string& line, std::ostream& out);
};
}  // namespace draughts
