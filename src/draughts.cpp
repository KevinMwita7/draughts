// draughts.cpp : Defines the entry point for the application.

#include "draughts/draughts.h"

int main() {
  draughts::MaterialEval eval;
  draughts::TextProtocol protocol(eval);
  protocol.run(std::cin, std::cout);
  return 0;
}
