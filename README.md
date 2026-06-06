# draughts

A C++ draughts (checkers) engine with alpha-beta search.

## Features

- Bitboard-based position representation (32-bit, one bit per playable square)
- Iterative-deepening alpha-beta search
- Pluggable evaluator interface (material and linear evaluation built in)
- Zobrist hashing
- Perft for move generation testing
- Text protocol for driving the engine via stdin/stdout

## Building

Requires CMake 3.14+ and a C++20 compiler.

```sh
cmake -B build
cmake --build build
```

The test suite is fetched and built automatically:

```sh
cd build && ctest
```

## Project Structure

```
include/draughts/   - public headers
src/                - engine source
test/               - GoogleTest unit tests
```

## License

MIT
