# ============================================================================
#  Alternative a CMake, sans dependance.
#    make            binaires en release dans bin/
#    make debug      avec sanitizers + assertions + traces
#    make test       suite de conformite
#    make bench      mesure de performance
#    make bundle     submit/tzaar_bot.cpp (fichier unique pour CodinGame)
# ============================================================================
CXX      ?= g++
STD      := -std=c++17
WARN     := -Wall -Wextra
REL      := -O3 -march=native -DNDEBUG
DBG      := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
            -DTZAAR_ASSERT=1 -DTZAAR_TRACE=1

SRC  := $(wildcard src/core/*.cpp src/engine/*.cpp src/io/*.cpp src/debug/*.cpp)
BIN  := bin

.PHONY: all debug test bench perft arena bundle clean
all: $(BIN)/tzaar $(BIN)/tzaar_arena $(BIN)/tzaar_bench $(BIN)/tzaar_perft $(BIN)/tzaar_tests

$(BIN):
	@mkdir -p $(BIN)

$(BIN)/tzaar:       $(SRC) src/main_cg.cpp        | $(BIN) ; $(CXX) $(STD) $(WARN) $(REL) -o $@ $^
$(BIN)/tzaar_arena: $(SRC) tools/arena.cpp        | $(BIN) ; $(CXX) $(STD) $(WARN) $(REL) -o $@ $^
$(BIN)/tzaar_bench: $(SRC) tools/bench.cpp        | $(BIN) ; $(CXX) $(STD) $(WARN) $(REL) -o $@ $^
$(BIN)/tzaar_perft: $(SRC) tools/perft.cpp        | $(BIN) ; $(CXX) $(STD) $(WARN) $(REL) -o $@ $^
$(BIN)/tzaar_tests: $(SRC) tests/test_rules.cpp   | $(BIN) ; $(CXX) $(STD) $(WARN) $(REL) -o $@ $^

debug: | $(BIN)
	$(CXX) $(STD) $(WARN) $(DBG) -o $(BIN)/tzaar_dbg   $(SRC) src/main_cg.cpp
	$(CXX) $(STD) $(WARN) $(DBG) -o $(BIN)/tzaar_tests_dbg $(SRC) tests/test_rules.cpp

test:  $(BIN)/tzaar_tests ; ./$(BIN)/tzaar_tests
perft: $(BIN)/tzaar_perft ; ./$(BIN)/tzaar_perft 4
bench: $(BIN)/tzaar_bench ; ./$(BIN)/tzaar_bench --time 40
arena: $(BIN)/tzaar_arena ; ./$(BIN)/tzaar_arena --games 20 --time-ms 40 --open 6

bundle: ; python3 tools/bundle.py

clean: ; rm -rf $(BIN)
