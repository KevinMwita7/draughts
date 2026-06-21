#pragma once
#include <bit>

#include "types.h"

// Square layout (0-indexed, bit N of a Bitboard = square N).
// BLACK starts on rows 0-2; WHITE starts on rows 5-7.
//
// Initial position (men only, no kings):
//   bb[BLACK][MAN] = 0x00000FFF  (bits  0-11)
//   bb[WHITE][MAN] = 0xFFF00000  (bits 20-31)
//   occupied       = 0xFFF00FFF
//
//
//        col:  0    1    2    3    4    5    6    7
//   row 7:          28        29        30        31   (WHITE back rank)
//   row 6:     24        25        26        27
//   row 5:          20        21        22        23
//   row 4:     16        17        18        19
//   row 3:          12        13        14        15
//   row 2:      8         9        10        11
//   row 1:           4         5         6         7
//   row 0:      0         1         2         3        (BLACK back rank)
//
// Each row has 4 playable squares. Square r*4+k occupies column 2k on even rows
// and column 2k+1 on odd rows (k in {0,1,2,3}).
//
// Shift derivation (UP-RIGHT worked example):
//   Even row (r, k):  dest = (r+1)*4 +  k    = r*4+k+4  => shift +4, no mask
//   needed Odd  row (r, k):  dest = (r+1)*4 + (k+1) = r*4+k+5  => shift +5
//     k=3 would target col 8 (off-board), so mask RIGHT_EDGE = {7,15,23,31}
//     first.
//
// DOWN-LEFT worked example:
//   Odd  row (r, k):  dest = (r-1)*4 +  k    = r*4+k-4  => shift -4, no mask
//   needed Even row (r, k):  dest = (r-1)*4 + (k-1) = r*4+k-5  => shift -5
//     k=0 would target col -1 (off-board), so mask LEFT_EDGE = {0,8,16,24}
//     first.
//
// The same logic gives all four shifts. Top and bottom limits need no masking:
// the uint32_t naturally discards bits that overflow above bit 31 (going past
// row 7) or shift right past bit 0 (going below row 0). Only left and right
// limits need explicit masking, because an out-of-bounds column step does NOT
// disappear; it wraps the bit to the adjacent row instead.
//
// LEFT_EDGE  = {0,8,16,24}  col-0 squares on even rows; no left-diagonal
// neighbour RIGHT_EDGE = {7,15,23,31} col-7 squares on odd rows;  no
// right-diagonal neighbour
//
//   shift_up_right:    (even             ) << 4  |  (odd & ~RIGHT_EDGE) << 5
//   shift_up_left:     (even & ~LEFT_EDGE) << 3  |  (odd              ) << 4
//   shift_down_right:  (odd & ~RIGHT_EDGE) >> 3  |  (even             ) >> 4
//   shift_down_left:   (odd              ) >> 4  |  (even & ~LEFT_EDGE) >> 5

namespace draughts {

// Single-square bitmask

constexpr Bitboard sq_bb(Square s) noexcept { return Bitboard(1) << s; }

// Rank (row) masks
// rank_bb(r) covers the four playable squares on row r (0 = bottom, 7 = top).
// Each row occupies exactly one nibble of the 32-bit Bitboard, so the mask is
// always 0xF shifted left by r*4.

constexpr Bitboard rank_bb(int r) noexcept { return Bitboard(0xF) << (r * 4); }

constexpr Bitboard ALL_SQUARES = 0xFFFFFFFFu;

constexpr Bitboard RANK[8] = {
    rank_bb(0), rank_bb(1), rank_bb(2), rank_bb(3),
    rank_bb(4), rank_bb(5), rank_bb(6), rank_bb(7),
};

constexpr Bitboard BLACK_PROMO_RANK = RANK[7];
constexpr Bitboard WHITE_PROMO_RANK = RANK[0];

// Row-parity masks

constexpr Bitboard EVEN_ROWS =
    RANK[0] | RANK[2] | RANK[4] | RANK[6];  // cols 0,2,4,6
constexpr Bitboard ODD_ROWS =
    RANK[1] | RANK[3] | RANK[5] | RANK[7];  // cols 1,3,5,7

// LEFT_EDGE: col-0 squares on even rows (no UP_LEFT or DOWN_LEFT neighbour).
constexpr Bitboard LEFT_EDGE = sq_bb(0) | sq_bb(8) | sq_bb(16) | sq_bb(24);
// RIGHT_EDGE: col-7 squares on odd rows (no UP_RIGHT or DOWN_RIGHT neighbour).
constexpr Bitboard RIGHT_EDGE = sq_bb(7) | sq_bb(15) | sq_bb(23) | sq_bb(31);

// Diagonal shift functions

constexpr Bitboard shift_up_right(Bitboard bb) noexcept {
  return ((bb & EVEN_ROWS) << 4) | ((bb & ODD_ROWS & ~RIGHT_EDGE) << 5);
}
constexpr Bitboard shift_up_left(Bitboard bb) noexcept {
  return ((bb & EVEN_ROWS & ~LEFT_EDGE) << 3) | ((bb & ODD_ROWS) << 4);
}
constexpr Bitboard shift_down_right(Bitboard bb) noexcept {
  return ((bb & ODD_ROWS & ~RIGHT_EDGE) >> 3) | ((bb & EVEN_ROWS) >> 4);
}
constexpr Bitboard shift_down_left(Bitboard bb) noexcept {
  return ((bb & ODD_ROWS) >> 4) | ((bb & EVEN_ROWS & ~LEFT_EDGE) >> 5);
}

// Shift in all four diagonal directions at once (useful for king attack sets).
constexpr Bitboard shift_all(Bitboard bb) noexcept {
  return shift_up_right(bb) | shift_up_left(bb) | shift_down_right(bb) |
         shift_down_left(bb);
}

// Bit utilities

constexpr int popcount(Bitboard bb) noexcept { return std::popcount(bb); }
constexpr Square lsb(Bitboard bb) noexcept {
  return Square(std::countr_zero(bb));
}
// Clears the lowest set bit and returns its square index.
constexpr Square pop_lsb(Bitboard& bb) noexcept {
  Square s = lsb(bb);
  bb &= bb - 1;
  return s;
}

// Coordinate conversion
// Board64to32[sq64] = -1 for non-playable squares.
// sq64 uses 0 = top-left of the 8x8 board (matches Board64to32 row order
// above).

constexpr int8_t Board64to32[64] = {
    -1, 28, -1, 29, -1, 30, -1, 31, 24, -1, 25, -1, 26, -1, 27, -1,
    -1, 20, -1, 21, -1, 22, -1, 23, 16, -1, 17, -1, 18, -1, 19, -1,
    -1, 12, -1, 13, -1, 14, -1, 15, 8,  -1, 9,  -1, 10, -1, 11, -1,
    -1, 4,  -1, 5,  -1, 6,  -1, 7,  0,  -1, 1,  -1, 2,  -1, 3,  -1,
};

// Board32to64[sq32] gives the corresponding 8x8 square index.
constexpr int8_t Board32to64[32] = {
    56, 58, 60, 62, 49, 51, 53, 55, 40, 42, 44, 46, 33, 35, 37, 39,
    24, 26, 28, 30, 17, 19, 21, 23, 8,  10, 12, 14, 1,  3,  5,  7,
};
}  // namespace draughts
