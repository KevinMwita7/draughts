# CMakeList.txt : CMake project for draughts, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("draughts")

# Add source to this project's executable.
add_executable (draughts
    "src/draughts.cpp"
    "include/draughts/types.h"
    "include/draughts/bitboard.h"
    "include/draughts/position.h"
    "include/draughts/movegen.h"
    "include/draughts/eval.h"
    "include/draughts/eval_material.h"
    "include/draughts/eval_linear.h"
    "include/draughts/search.h"
    "include/draughts/notation.h"
    "include/draughts/protocol.h"
    "include/draughts/perft.h"
)

target_include_directories(draughts PRIVATE include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET draughts PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
