#pragma once
#include "types.h"
#include <cstdint>
#include <vector>

namespace draughts {

enum class TTFlag : uint8_t {
    EXACT       = 0,
    LOWER_BOUND = 1,
    UPPER_BOUND = 2,
};

struct TTEntry {
    uint32_t key;        // upper 32 bits of Zobrist hash — collision guard
    int16_t  score;
    uint8_t  move_from;  // SQ_NONE if no best move stored
    uint8_t  move_to;
    uint8_t  depth;
    TTFlag   flag;
    uint8_t  gen;        // generation counter at time of store
    uint8_t  _pad;
};
static_assert(sizeof(TTEntry) == 12);

class TranspositionTable {
public:
    // n_entries must be a power of two.
    explicit TranspositionTable(uint32_t n_entries);

    void resize(uint32_t n_entries);
    void clear();
    void new_search();

    // Returns true on a hit and populates *out.
    bool probe(uint64_t hash, TTEntry* out) const;

    // Stores under the depth-preferred replacement policy.
    void store(uint64_t hash, Score score, Move best_move,
               uint8_t depth, TTFlag flag);

    // Permille fill (0-1000), sampled over the first min(1000, size) entries.
    // https://chess.stackexchange.com/questions/38815/what-does-hashfull-1000-mean-in-stockfish
    int hashfull() const;

private:
    std::vector<TTEntry> table_;
    uint32_t             mask_;
    uint8_t              current_gen_ = 0;
};

} // namespace draughts
