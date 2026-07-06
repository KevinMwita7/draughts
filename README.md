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

## Building for WebAssembly

The `wasm-debug` CMake preset builds with [Emscripten](https://emscripten.org/). Install and activate the SDK first:

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest --permanent   # persists the EMSDK env var for future shells
source ./emsdk_env.sh                 # emsdk_env.bat on Windows; sets it for the current shell
```

The preset's `toolchainFile` resolves via the `EMSDK` environment variable (`$env{EMSDK}`) rather than a hardcoded path, so it works regardless of where emsdk is installed or which OS you're on. Configure and build like any other preset:

```sh
cmake --preset wasm-debug
cmake --build --preset wasm-debug
```

**Windows note:** `--permanent` writes `EMSDK` to the registry, but already-running processes (including an open IDE, or Explorer and anything it launches) won't see it until you sign out/in or reboot. If CMake reports it can't find the toolchain file right after activating emsdk, that's why — restarting the IDE alone isn't enough.

If you need a machine-specific override (e.g. `EMSDK` isn't set in your IDE's environment and you'd rather not sign out), create a `CMakeUserPresets.json` in the repo root — it's gitignored and meant for exactly this. Example:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "wasm-debug-local",
      "inherits": "wasm-debug",
      "environment": {
        "EMSDK": "C:/path/to/your/emsdk"
      }
    }
  ]
}
```

Then select `wasm-debug-local` as the configure preset instead.

## Project Structure

```
include/draughts/   - public headers
src/                - engine source
test/               - GoogleTest unit tests
```

## License

MIT
