// ============================================================================
//  Banc de mesure. Deux usages :
//    bench            : profondeur fixe sur quelques positions -> noeuds/s
//    bench --time T   : budget fixe -> profondeur atteinte
//  Utile pour comparer objectivement deux optimisations.
// ============================================================================
#include "../src/core/position.hpp"
#include "../src/engine/search.hpp"
#include "../src/engine/eval_registry.hpp"
#include "../src/engine/timer.hpp"
#include "../src/debug/trace.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

using namespace tzaar;

// Quelques positions representatives : ouverture, milieu, finale.
static std::vector<std::pair<Position,int>> make_suite() {
    std::vector<std::pair<Position,int>> out;
    std::mt19937_64 rng(12345);
    for (int nPlies : { 0, 8, 20, 34, 48 }) {
        Position p; p.init_start();
        int side = 0, phase = PHASE_CAPTURE;
        bool first = true;
        for (int k = 0; k < nPlies; ++k) {
            if (p.missing_type(side) >= 0) break;
            Move buf[MAX_MOVES];
            int n = p.gen_moves(buf, side, phase);
            if (n == 0) break;
            Move m = buf[rng() % (u64)n];
            if (!is_wait(m)) { Undo u; p.do_move(m, u); }
            if (phase == PHASE_CAPTURE && first) { side = 1; first = false; }
            else if (phase == PHASE_CAPTURE)     { phase = PHASE_FREE; }
            else { phase = PHASE_CAPTURE; side ^= 1; }
        }
        if (p.missing_type(side) < 0 && p.has_capture(side)) out.push_back({p, side});
    }
    return out;
}

int main(int argc, char** argv) {
    int depth = 6;
    i64 timeUs = 0;
    std::vector<std::string> ov;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto next = [&]() { return i + 1 < argc ? argv[++i] : (char*)"0"; };
        if      (s == "--depth") depth = std::atoi(next());
        else if (s == "--time")  timeUs = (i64)std::atoi(next()) * 1000;
        else if (s == "--set")   ov.push_back(next());
    }
    for (auto& kv : ov) {
        auto eq = kv.find('=');
        if (eq != std::string::npos && !set_param(params(), kv.substr(0, eq), std::atoi(kv.c_str() + eq + 1)))
            std::printf("parametre inconnu : %s\n", kv.substr(0, eq).c_str());
    }
    eval_rebuild_tables();

    auto suite = make_suite();
    Searcher eng; eng.set_hash_mb(64);

    u64 totalNodes = 0; double totalMs = 0; int sumDepth = 0;
    std::printf("%-6s %-8s %-10s %-12s %-8s %-10s %s\n",
                "pos", "piles", "prof.", "noeuds", "ms", "kN/s", "score");
    int i = 0;
    for (auto& pr : suite) {
        eng.new_game();
        SearchLimits lim;
        if (timeUs) { lim.timeUs = timeUs; lim.maxDepth = 64; }
        else        { lim.timeUs = 1000000000; lim.maxDepth = depth; }
        SearchStats st;
        eng.think(pr.first, pr.second, PHASE_CAPTURE, false, lim, st);
        totalNodes += st.nodes; totalMs += st.elapsedMs; sumDepth += st.depth;
        std::printf("%-6d %-8d %-10d %-12llu %-8.1f %-10.0f %-8d ordre=%.0f%% ttHit=%.0f%%\n",
                    i++, popcnt(pr.first.occ), st.depth,
                    (unsigned long long)st.nodes, st.elapsedMs,
                    st.elapsedMs > 0 ? st.nodes / st.elapsedMs : 0.0, st.score,
                    st.betaCuts ? 100.0 * st.firstCuts / st.betaCuts : 0.0,
                    st.nodes ? 100.0 * st.ttHits / st.nodes : 0.0);
    }
    std::printf("\nTOTAL : %llu noeuds en %.1f ms  ->  %.0f kN/s   (profondeur moyenne %.1f)\n",
                (unsigned long long)totalNodes, totalMs,
                totalMs > 0 ? totalNodes / totalMs : 0.0, (double)sumDepth / (suite.empty()?1:suite.size()));
    std::printf("params : %s\n", dump_params(params()).c_str());
    return 0;
}
