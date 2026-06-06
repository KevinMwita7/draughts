#include <random>
#include "draughts/zobrist.h"
#include "draughts/position.h"

using namespace std;

uint64_t draughts::zobrist::PIECE_KEY[32][4] = {0};
uint64_t draughts::zobrist::SIDE_KEY = 0;

void draughts::zobrist::init() {

	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<uint64_t> dist(0, numeric_limits<uint64_t>::max());

	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 4; ++j) {
			PIECE_KEY[i][j] = dist(gen);
		}
	}

	SIDE_KEY = dist(gen);
}

uint64_t draughts::compute_hash(const Position& pos) noexcept {
	uint64_t hash = 0;
	Bitboard occ = pos.occupied;

	if (pos.side_to_move == WHITE) {
		hash ^= zobrist::SIDE_KEY;
	}

	while (occ) {
		Square s = pop_lsb(occ);
		hash ^= zobrist::piece_key(s, pos.piece_on(s));
	}

	return hash;
}
