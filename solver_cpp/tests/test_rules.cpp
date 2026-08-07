// ============================================================================
//  Suite de conformite : la version rapide doit etre indiscernable du referee.
//  Renvoie 0 si tout passe.
// ============================================================================
#include "../src/core/position.hpp"
#include "../src/debug/trace.hpp"
#include "reference_rules.hpp"

#include <cstdio>
#include <algorithm>
#include <random>
#include <set>

using namespace tzaar;

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { \
    std::printf("  [FAIL] " __VA_ARGS__); std::printf("   (%s:%d)\n", __FILE__, __LINE__); ++g_fail; } } while (0)

// --------------------------------------------------------------- geometrie --
static void test_geometry() {
    std::printf("[geometrie]\n");
    const Geometry& g = GEO();
    int n = 0;
    for (int x = 0; x < 9; ++x) for (int y = 0; y < 9; ++y) if (g.index[x][y] >= 0) ++n;
    CHECK(n == 60, "60 cases attendues, %d trouvees\n", n);
    CHECK(g.index[4][4] == -1, "le centre doit etre absent\n");

    // Monotonie de l'indice : indispensable pour l'astuce lsb/msb.
    for (int c = 0; c < NUM_CELLS; ++c) {
        for (int d = 0; d < 3; ++d) {
            u64 m = g.ray[c][d];
            while (m) { int t = pop_lsb(m); CHECK(t > c, "dir %d non croissante (%d->%d)\n", d, c, t); }
        }
        for (int d = 3; d < 6; ++d) {
            u64 m = g.ray[c][d];
            while (m) { int t = pop_lsb(m); CHECK(t < c, "dir %d non decroissante (%d->%d)\n", d, c, t); }
        }
    }

    // Repartition des anneaux : 6/12/18/24
    int ringCount[5] = {0,0,0,0,0};
    for (int c = 0; c < NUM_CELLS; ++c) ringCount[g.ring[c]]++;
    CHECK(ringCount[1] == 6 && ringCount[2] == 12 && ringCount[3] == 18 && ringCount[4] == 24,
          "anneaux %d/%d/%d/%d\n", ringCount[1], ringCount[2], ringCount[3], ringCount[4]);
}

// --------------------------------------------------------- position initiale --
static void test_start_position() {
    std::printf("[position initiale]\n");
    Position p; p.init_start();
    CHECK(popcnt(p.occ) == 60, "60 piles attendues\n");
    for (int s = 0; s < 2; ++s) {
        CHECK(p.counts[s][TOTT]   == 15, "J%d: 15 TOTT attendus, %d\n", s, p.counts[s][TOTT]);
        CHECK(p.counts[s][TZARRA] ==  9, "J%d: 9 TZARRA attendus, %d\n",  s, p.counts[s][TZARRA]);
        CHECK(p.counts[s][TZAAR]  ==  6, "J%d: 6 TZAAR attendus, %d\n",   s, p.counts[s][TZAAR]);
        CHECK(p.pieces[s] == 30, "J%d: 30 pieces attendues, %d\n", s, p.pieces[s]);
    }
    // L'anneau 2 est celui des TZAAR (cf. Board.initNonRandom).
    for (int c = 0; c < NUM_CELLS; ++c)
        if (GEO().ring[c] == 2) CHECK(p.type_[c] == TZAAR, "case %d de l'anneau 2 devrait etre TZAAR\n", c);
    std::string why;
    CHECK(check_invariants(p, &why), "invariants: %s\n", why.c_str());
}

// ------------------------------------------------------------------ movegen --
static bool same_move_set(std::vector<Move> a, std::vector<Move> b) {
    std::sort(a.begin(), a.end()); std::sort(b.begin(), b.end());
    return a == b;
}

static void test_movegen_against_reference(std::mt19937_64& rng) {
    std::printf("[movegen vs oracle]\n");
    int positions = 0, mismatches = 0;

    for (int game = 0; game < 60; ++game) {
        Position p; p.init_start();
        int side = 0, phase = PHASE_CAPTURE;
        bool firstAction = true;

        for (int step = 0; step < 240; ++step) {
            if (p.missing_type(side) >= 0) break;
            if (phase == PHASE_CAPTURE && !p.has_capture(side)) break;

            for (int ph = 1; ph <= 2; ++ph) {
                for (int sd = 0; sd < 2; ++sd) {
                    Move fast[MAX_MOVES];
                    int nf = p.gen_moves(fast, sd, ph);
                    std::vector<Move> vf(fast, fast + nf);
                    std::vector<Move> vr = ref::gen_moves(p, sd, ph);
                    ++positions;
                    if (!same_move_set(vf, vr)) {
                        ++mismatches;
                        if (mismatches < 3) {
                            std::printf("  divergence side=%d phase=%d (%zu rapide / %zu oracle)\n",
                                        sd, ph, vf.size(), vr.size());
                            std::printf("%s", board_to_string(p).c_str());
                        }
                    }
                    // is_legal doit accepter exactement l'ensemble genere.
                    std::set<Move> gen(vr.begin(), vr.end());
                    for (int f = 0; f < NUM_CELLS; ++f)
                        for (int t = 0; t < NUM_CELLS; ++t) {
                            Move m = mk_move(f, t);
                            bool exp = gen.count(m) > 0;
                            if (p.is_legal(m, sd, ph) != exp) {
                                std::printf("  is_legal incoherent m=%d,%d side=%d phase=%d\n", f, t, sd, ph);
                                ++g_fail; return;
                            }
                        }
                }
            }

            Move buf[MAX_MOVES];
            int n = p.gen_moves(buf, side, phase);
            if (n == 0) break;
            Move m = buf[rng() % (u64)n];

            // Invariants sur do/undo.
            u64 h0 = p.hash;
            Undo u; p.do_move(m, u);
            std::string why;
            if (!check_invariants(p, &why)) { std::printf("  invariants apres coup: %s\n", why.c_str()); ++g_fail; return; }
            Position after = p;
            p.undo_move(u);
            if (p.hash != h0) { std::printf("  hash non restaure\n"); ++g_fail; return; }
            if (!check_invariants(p, &why)) { std::printf("  invariants apres annulation: %s\n", why.c_str()); ++g_fail; return; }
            p = after;

            if (phase == PHASE_CAPTURE && firstAction && side == 0) { side = 1; firstAction = false; }
            else if (phase == PHASE_CAPTURE) { phase = PHASE_FREE; firstAction = false; }
            else { phase = PHASE_CAPTURE; side ^= 1; }
        }
    }
    std::printf("  %d positions comparees, %d divergences\n", positions, mismatches);
    CHECK(mismatches == 0, "movegen diverge de l'oracle\n");
}

// ------------------------------------------------ regles fines du referee ----
static void test_edge_cases() {
    std::printf("[cas limites]\n");
    Position p; p.clear();

    // 1) Le centre bloque : deux cases opposees alignees a travers (0,0).
    int a = GEO().index[4 - 1][4 + 0];   // (-1, 0)
    int b = GEO().index[4 + 1][4 + 0];   // (+1, 0)
    p.put(a, 0, TOTT, 1);
    p.put(b, 1, TOTT, 1);
    p.refresh();
    CHECK(!p.is_legal(mk_move(a, b), 0, PHASE_CAPTURE), "le trou central doit bloquer la ligne\n");
    CHECK(!p.has_capture(0), "aucune capture ne doit etre possible a travers le centre\n");

    // 2) Capture : hauteur egale autorisee, inferieure interdite.
    p.clear();
    int c1 = GEO().index[4 + 0][4 - 4];  // (0,-4)
    int c2 = GEO().index[4 + 0][4 - 3];  // (0,-3)  voisin direct
    p.put(c1, 0, TOTT, 2); p.put(c2, 1, TZAAR, 2); p.refresh();
    CHECK(p.is_legal(mk_move(c1, c2), 0, PHASE_CAPTURE), "2 vs 2 doit capturer\n");
    p.clear(); p.put(c1, 0, TOTT, 1); p.put(c2, 1, TZAAR, 2); p.refresh();
    CHECK(!p.is_legal(mk_move(c1, c2), 0, PHASE_CAPTURE), "1 vs 2 ne doit pas capturer\n");

    // 3) Capture : la pile qui prend NE gagne PAS de hauteur.
    p.clear(); p.put(c1, 0, TOTT, 3); p.put(c2, 1, TZAAR, 2); p.refresh();
    { Undo u; p.do_move(mk_move(c1, c2), u);
      CHECK(p.height_[c2] == 3, "hauteur apres capture = 3, obtenu %d\n", p.height_[c2]);
      CHECK(p.type_[c2] == TOTT && p.owner_[c2] == 0, "la pile capturante conserve son type\n");
      CHECK(p.counts[1][TZAAR] == 0, "le TZAAR adverse doit avoir disparu\n"); }

    // 4) Empilement : hauteurs additionnees, type = celui de la pile qui arrive.
    //    Consequence : enterrer sa derniere piece d'un type = defaite.
    p.clear(); p.put(c1, 0, TZAAR, 2); p.put(c2, 0, TOTT, 3); p.refresh();
    { Undo u; p.do_move(mk_move(c1, c2), u);
      CHECK(p.height_[c2] == 5, "hauteur apres empilement = 5, obtenu %d\n", p.height_[c2]);
      CHECK(p.type_[c2] == TZAAR, "le type devient celui de la pile qui arrive\n");
      CHECK(p.counts[0][TOTT] == 0, "le TOTT enterre ne compte plus\n");
      CHECK(p.missing_type(0) == TOTT, "enterrer son dernier TOTT doit etre une defaite\n"); }

    // 5) L'empilement est interdit en phase 1.
    p.clear(); p.put(c1, 0, TZAAR, 1); p.put(c2, 0, TOTT, 1); p.refresh();
    CHECK(!p.is_legal(mk_move(c1, c2), 0, PHASE_CAPTURE), "pas d'empilement en phase 1\n");
    CHECK( p.is_legal(mk_move(c1, c2), 0, PHASE_FREE),    "empilement autorise en phase 2\n");

    // 6) WAIT n'est legal qu'en phase 2.
    CHECK(!p.is_legal(MOVE_WAIT, 0, PHASE_CAPTURE), "WAIT interdit en phase 1\n");
    CHECK( p.is_legal(MOVE_WAIT, 0, PHASE_FREE),    "WAIT autorise en phase 2\n");

    // 7) Une pile ne peut pas sauter par-dessus une autre.
    p.clear();
    int d0 = GEO().index[4 + 0][4 - 4], d1 = GEO().index[4 + 0][4 - 3], d2 = GEO().index[4 + 0][4 - 2];
    p.put(d0, 0, TOTT, 5); p.put(d1, 0, TOTT, 1); p.put(d2, 1, TOTT, 1); p.refresh();
    CHECK(!p.is_legal(mk_move(d0, d2), 0, PHASE_CAPTURE), "impossible de sauter une pile\n");
}

// ------------------------------------------------------- serialisation -------
static void test_roundtrip() {
    std::printf("[serialisation]\n");
    Position p; p.init_start();
    Position q;
    CHECK(position_from_input(q, position_to_input(p)), "relecture du dump\n");
    CHECK(q.hash == p.hash, "hash different apres aller-retour\n");
}

int main() {
    std::mt19937_64 rng(20260805);
    test_geometry();
    test_start_position();
    test_edge_cases();
    test_movegen_against_reference(rng);
    test_roundtrip();
    if (g_fail == 0) std::printf("\nTOUS LES TESTS PASSENT\n");
    else             std::printf("\n%d ECHEC(S)\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
