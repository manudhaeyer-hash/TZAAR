// ============================================================================
//  perft : compte les feuilles de l'arbre a N demi-actions et mesure le debit
//  de la generation de coups + make/unmake. Sert de non-regression (les
//  nombres doivent rester identiques) et de mesure d'optimisation.
//
//  Usage : perft [profondeur_max]
// ============================================================================
#include "../src/core/position.hpp"
#include "../src/engine/timer.hpp"
#include <cstdio>
#include <cstdlib>

using namespace tzaar;

static u64 g_terminal = 0;

static u64 perft(Position& p, int side, int phase, int depth, bool firstAction) {
    if (p.missing_type(side) >= 0) { ++g_terminal; return 1; }
    if (phase == PHASE_CAPTURE && !p.has_capture(side)) { ++g_terminal; return 1; }
    if (depth == 0) return 1;

    Move buf[MAX_MOVES];
    int n = p.gen_moves(buf, side, phase);
    u64 total = 0;
    for (int i = 0; i < n; ++i) {
        Undo u; p.do_move(buf[i], u);
        int ns, np; bool nf = false;
        if (phase == PHASE_CAPTURE && firstAction) { ns = side ^ 1; np = PHASE_CAPTURE; }
        else if (phase == PHASE_CAPTURE)           { ns = side;     np = PHASE_FREE; }
        else                                       { ns = side ^ 1; np = PHASE_CAPTURE; }
        total += perft(p, ns, np, depth - 1, nf);
        p.undo_move(u);
    }
    return total;
}

int main(int argc, char** argv) {
    int maxDepth = argc > 1 ? std::atoi(argv[1]) : 5;
    Position p; p.init_start();

    std::printf("Position initiale : %d piles\n", popcnt(p.occ));
    std::printf("%-7s %-16s %-12s %-12s %s\n", "prof.", "feuilles", "terminaux", "temps(ms)", "feuilles/s");
    for (int d = 1; d <= maxDepth; ++d) {
        g_terminal = 0;
        Timer t; t.start();
        u64 n = perft(p, 0, PHASE_CAPTURE, d, true);
        double ms = t.ms();
        std::printf("%-7d %-16llu %-12llu %-12.1f %.0f\n", d,
                    (unsigned long long)n, (unsigned long long)g_terminal, ms,
                    ms > 0 ? n / (ms / 1000.0) : 0.0);
    }
    return 0;
}
