// ============================================================================
//  Point d'entree du bot CodinGame.
//
//  Budget temps : 1000 ms au premier tour, 50 ms ensuite. On garde une marge
//  confortable (l'I/O et la JVM du referee mangent quelques ms).
//
//  Variables d'environnement (ignorees sur CodinGame, utiles en local) :
//    TZAAR_TIME_MS   budget par tour, defaut 40 (limite CodinGame : 50)
//    TZAAR_HASH_MB   taille de la table de transposition, defaut 64
//    TZAAR_DEPTH     profondeur max en demi-actions, defaut 64
// ============================================================================
#include "core/position.hpp"
#include "engine/search.hpp"
#include "io/protocol.hpp"
#include "debug/trace.hpp"

#include <iostream>
#include <cstdlib>

using namespace tzaar;

static int env_int(const char* k, int def) {
    const char* v = std::getenv(k);
    return v ? std::atoi(v) : def;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int myId = 0;
    if (!(std::cin >> myId)) return 0;

    Searcher searcher;
    searcher.set_hash_mb((size_t)env_int("TZAAR_HASH_MB", 64));
    searcher.new_game();

    const i64 turnUs  = (i64)env_int("TZAAR_TIME_MS", 40) * 1000;
    const i64 firstUs = (i64)env_int("TZAAR_FIRST_MS", 900) * 1000;
    const int maxDepth = env_int("TZAAR_DEPTH", 64);

    int action = 0;      // numero de la demi-action jouee par nous
    bool firstEver = true;

    for (;;) {
        TurnInput t = read_turn(std::cin);
        if (!t.ok) break;

        // Cas particulier du referee : a la toute premiere action de la partie,
        // le joueur 0 joue la phase 1 puis la main passe directement.
        bool firstActionOfGame = firstEver && myId == 0 && t.phase == PHASE_CAPTURE;

        SearchLimits lim;
        lim.maxDepth = maxDepth;
        lim.timeUs   = firstEver ? firstUs : turnUs;
        SearchStats st;

        Move m = searcher.think(t.pos, myId, t.phase, firstActionOfGame, lim, st);

        // Filet de securite : jamais de sortie invalide.
        if (!t.pos.is_legal(m, myId, t.phase)) {
            Move buf[MAX_MOVES];
            int n = t.pos.gen_moves(buf, myId, t.phase);
            m = n > 0 ? buf[0] : MOVE_WAIT;
        }

        std::cout << move_to_string(m) << std::endl;

        std::cerr << "d=" << st.depth << "/" << st.seldepth
                  << " sc=" << st.score
                  << " n=" << st.nodes
                  << " " << st.elapsedMs << "ms"
                  << (st.proven ? " [PROUVE]" : "")
                  << " pv: " << st.pv << std::endl;

        firstEver = false;
        ++action;
    }
    return 0;
}
