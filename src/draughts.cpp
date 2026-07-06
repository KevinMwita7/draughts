// draughts.cpp : Defines the entry point for the application.

#include "draughts/draughts.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

#include <sstream>

extern "C" {

// JS entry point: one line of TextProtocol input in, full text output back.
// Lazily owns the engine session so it persists across calls from the
// browser (main() doesn't run a blocking loop under Emscripten).
EMSCRIPTEN_KEEPALIVE
const char* dr_command(const char* line) {
  static draughts::LinearEval eval;
  static draughts::TextProtocol protocol(eval);
  static std::string response;

  std::ostringstream oss;
  protocol.handle_command(line, oss);
  response = oss.str();
  return response.c_str();
}

}  // extern "C"
#endif

int main() {
#ifndef __EMSCRIPTEN__
  draughts::LinearEval eval;
  draughts::TextProtocol protocol(eval);
  protocol.run(std::cin, std::cout);
#endif
  return 0;
}
