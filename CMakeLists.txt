# CMakeList.txt : CMake project for draughts, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("draughts")

add_library(draughts_core STATIC
    "src/zobrist.cpp"
    "src/position.cpp"
    "src/tt.cpp"
    "src/movegen.cpp"
    "include/draughts/types.h"
    "include/draughts/tt.h"
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

target_include_directories(draughts_core PUBLIC include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET draughts_core PROPERTY CXX_STANDARD 20)
endif()

add_executable(draughts "src/draughts.cpp" "src/movegen.cpp")
target_link_libraries(draughts PRIVATE draughts_core)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET draughts PROPERTY CXX_STANDARD 20)
endif()

enable_testing()
add_subdirectory(test)
