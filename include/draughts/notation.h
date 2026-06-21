#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "position.h"

namespace draughts {

// PDN position string
// Format: "<side>:W<sq>,<sq>,...:B<sq>,<sq>,..."
// Squares use the standard 1-indexed PDN convention (PDN sq = internal sq + 1).
// Kings are prefixed with 'K', e.g. "K5".
// Example start position:
//   "B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12"

Position parse_pdn_position(std::string_view pdn);
std::string to_pdn_position(const Position& pos);

// ---- Move text --------------------------------------------------------------
// Quiet move: "1-5"  (PDN squares, 1-indexed)
// Capture:    "1x10" or "1x10x19" for multi-jump
// parse_move needs the position to resolve ambiguity.

Move parse_move(std::string_view text, const Position& pos);
std::string move_to_string(Move m);

// ---- PDN game records -------------------------------------------------------
// Parses a PDN game string and returns the move list.
// Starts from 'start'; defaults to the standard opening position.

std::vector<Move> parse_pdn(std::string_view pdn,
                            Position start = Position::start_position());

}  // namespace draughts
