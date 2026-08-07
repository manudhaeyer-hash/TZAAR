cmake_minimum_required(VERSION 3.16)
project(tzaar_solver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(TZAAR_NATIVE  "Compiler pour le CPU local (-march=native)" ON)
option(TZAAR_TRACE   "Activer les traces de recherche sur stderr"  OFF)
option(TZAAR_SANITIZE "Activer ASan/UBSan (build Debug)"           OFF)

file(GLOB CORE   src/core/*.cpp)
file(GLOB ENGINE src/engine/*.cpp)
file(GLOB IO     src/io/*.cpp)
file(GLOB DEBUGS src/debug/*.cpp)
add_library(tzaar_core STATIC ${CORE} ${ENGINE} ${IO} ${DEBUGS})
target_include_directories(tzaar_core PUBLIC src)

if(TZAAR_NATIVE AND NOT MSVC)
  target_compile_options(tzaar_core PUBLIC -march=native)
endif()
if(NOT MSVC)
  target_compile_options(tzaar_core PUBLIC -Wall -Wextra)
endif()
if(TZAAR_TRACE)
  target_compile_definitions(tzaar_core PUBLIC TZAAR_TRACE=1)
endif()
if(TZAAR_SANITIZE AND NOT MSVC)
  target_compile_options(tzaar_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer -DTZAAR_ASSERT=1)
  target_link_options(tzaar_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(tzaar        src/main_cg.cpp)   # le bot (protocole CodinGame)
add_executable(tzaar_arena  tools/arena.cpp)   # self-play / A-B des parametres
add_executable(tzaar_bench  tools/bench.cpp)   # mesure noeuds/s et profondeur
add_executable(tzaar_perft  tools/perft.cpp)   # conformite + debit de la movegen
add_executable(tzaar_tests  tests/test_rules.cpp)
foreach(t tzaar tzaar_arena tzaar_bench tzaar_perft tzaar_tests)
  target_link_libraries(${t} PRIVATE tzaar_core)
endforeach()

enable_testing()
add_test(NAME regles COMMAND tzaar_tests)
