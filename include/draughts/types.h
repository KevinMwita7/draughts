# pragma once
# include <cstdint>

namespace draughts {
	// Index of a bit in a Bitboard (0–31)
	using Square = std::uint8_t;
	// 32 bits map 1:1 to the 32 playable squares; bit N set means square N is occupied
	using Bitboard = std::uint32_t;

	enum Color : std::uint8_t { Black, White };
	enum PieceKind : std::uint8_t { Man, King };

	struct Move {
		Square from;
		Square to;
		Bitboard captured;
		bool promotion;
	};
}
