// ============================================================================
//  Arene de self-play. Le referee est reimplemente ici a l'identique du Java
//  (Referee.gameTurn), ce qui permet de jouer des milliers de parties sans JVM.
//
//  Sert a trois choses :
//    1. non-regression : aucune sortie illegale, aucun depassement de temps ;
//    2. tuning : A/B entre deux jeux de parametres d'evaluation ;
//    3. mesure : longueur des parties, temps max par coup, causes de fin.
//
//  Usage :
//    arena --games 40 --time-ms 40 [--open 4] [--seed 1] [--hash 32]
//          [--a nom=valeur ...] [--b nom=valeur ...] [--b-random] [--verbose]
//
//  Les parties partant toujours de la meme position, --open N joue les N
//  premieres demi-actions au hasard pour diversifier l'echantillon.
// ============================================================================
#include "../src/core/position.hpp"
#include "../src/engine/search.hpp"
#include "../src/engine/eval_registry.hpp"
#include "../src/debug/trace.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

using namespace tzaar;

static const int MAX_TURNS = 400;   // identique a Referee.MAX_TURNS

enum EndReason { END_MISSING, END_NO_CAPTURE, END_ILLEGAL, END_TURNS };
static const char* REASON[] = { "type elimine", "aucune capture", "COUP ILLEGAL", "limite de tours" };

struct Agent {
    EvalParams params;
    bool       randomPlay = false;
    Searcher   engine;
    double     maxMs = 0;
    double     sumMs = 0;
    u64        moves = 0;
    u64        nodes = 0;
};

struct GameResult { int winner; EndReason reason; int actions; };

static GameResult play_game(Agent& a0, Agent& a1, i64 timeUs, int openPlies,
                            std::mt19937_64& rng, bool verbose) {
    Position pos; pos.init_start();
    Agent* ag[2] = { &a0, &a1 };
    ag[0]->engine.new_game();
    ag[1]->engine.new_game();

    int side = 0, phase = PHASE_CAPTURE, totalTurns = 0;

    for (int t = 0; t < MAX_TURNS; ++t) {
        if (pos.missing_type(side) >= 0)                     return { side ^ 1, END_MISSING,    totalTurns };
        if (phase == PHASE_CAPTURE && !pos.has_capture(side)) return { side ^ 1, END_NO_CAPTURE, totalTurns };

        Move m;
        if (totalTurns < openPlies) {
            Move buf[MAX_MOVES];
            int n = pos.gen_moves(buf, side, phase);
            m = buf[rng() % (u64)n];
        } else if (ag[side]->randomPlay) {
            Move buf[MAX_MOVES];
            int n = pos.gen_moves(buf, side, phase);
            m = buf[rng() % (u64)n];
        } else {
            params() = ag[side]->params;            // parametres propres a l'agent
            SearchLimits lim; lim.timeUs = timeUs;
            SearchStats st;
            bool firstAction = (totalTurns == 0);
            m = ag[side]->engine.think(pos, side, phase, firstAction, lim, st);
            ag[side]->maxMs = std::max(ag[side]->maxMs, st.elapsedMs);
            ag[side]->sumMs += st.elapsedMs;
            ag[side]->moves++;
            ag[side]->nodes += st.nodes;
        }

        if (!pos.is_legal(m, side, phase)) {
            std::printf("!! coup illegal de J%d : %s (phase %d)\n%s",
                        side, move_to_string(m).c_str(), phase, board_to_string(pos).c_str());
            return { side ^ 1, END_ILLEGAL, totalTurns };
        }

        if (verbose) std::printf("  t%-3d J%d p%d  %s\n", totalTurns, side, phase, move_to_string(m).c_str());

        if (!is_wait(m)) { Undo u; pos.do_move(m, u); }

        // Enchainement exact du referee.
        if (phase == PHASE_CAPTURE) {
            if (totalTurns == 0) { side = 1; phase = PHASE_CAPTURE; }
            else                 { phase = PHASE_FREE; }
        } else {
            phase = PHASE_CAPTURE; side ^= 1;
        }
        ++totalTurns;
    }
    return { -1, END_TURNS, totalTurns };
}

static void apply_overrides(EvalParams& p, const std::vector<std::string>& kv) {
    for (const auto& s : kv) {
        auto eq = s.find('=');
        if (eq == std::string::npos) { std::printf("option ignoree : %s\n", s.c_str()); continue; }
        std::string k = s.substr(0, eq);
        int v = std::atoi(s.c_str() + eq + 1);
        if (!set_param(p, k, v)) std::printf("parametre inconnu : %s\n", k.c_str());
    }
}

int main(int argc, char** argv) {
    int games = 20, openPlies = 4, hashMb = 32;
    i64 timeUs = 40000;
    u64 seed = 1;
    bool verbose = false, bRandom = false;
    std::vector<std::string> ovA, ovB;

    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto next = [&]() { return i + 1 < argc ? argv[++i] : (char*)"0"; };
        if      (s == "--games")   games   = std::atoi(next());
        else if (s == "--time-ms") timeUs  = (i64)std::atoi(next()) * 1000;
        else if (s == "--open")    openPlies = std::atoi(next());
        else if (s == "--seed")    seed    = (u64)std::atoll(next());
        else if (s == "--hash")    hashMb  = std::atoi(next());
        else if (s == "--verbose") verbose = true;
        else if (s == "--b-random") bRandom = true;
        else if (s == "--a")       ovA.push_back(next());
        else if (s == "--b")       ovB.push_back(next());
        else if (s == "--list-params") {
            for (const auto& n : param_names()) std::printf("%s\n", n.c_str());
            return 0;
        } else { std::printf("option inconnue : %s\n", s.c_str()); return 1; }
    }

    Agent A, B;
    apply_overrides(A.params, ovA);
    apply_overrides(B.params, ovB);
    B.randomPlay = bRandom;
    A.engine.set_hash_mb((size_t)hashMb);
    B.engine.set_hash_mb((size_t)hashMb);

    std::printf("A : %s\n", dump_params(A.params).c_str());
    std::printf("B : %s%s\n", bRandom ? "[aleatoire] " : "", dump_params(B.params).c_str());
    std::printf("%d parties, %lld ms/coup, %d demi-actions d'ouverture aleatoire\n\n",
                games, (long long)(timeUs / 1000), openPlies);

    int winA = 0, winB = 0, draws = 0, illegal = 0;
    int reasons[4] = {0,0,0,0};
    long long totalActions = 0;
    std::mt19937_64 rng(seed);

    for (int g = 0; g < games; ++g) {
        // On alterne les couleurs pour neutraliser l'avantage du trait.
        bool aIsFirst = (g % 2 == 0);
        Agent& p0 = aIsFirst ? A : B;
        Agent& p1 = aIsFirst ? B : A;
        GameResult r = play_game(p0, p1, timeUs, openPlies, rng, verbose);
        reasons[r.reason]++;
        totalActions += r.actions;
        if (r.reason == END_ILLEGAL) ++illegal;
        if (r.winner < 0) ++draws;
        else if ((r.winner == 0) == aIsFirst) ++winA;
        else ++winB;
        std::printf("partie %3d : %-6s en %3d demi-actions (%s)\n", g + 1,
                    r.winner < 0 ? "nulle" : ((r.winner == 0) == aIsFirst ? "A" : "B"),
                    r.actions, REASON[r.reason]);
        std::fflush(stdout);
    }

    std::printf("\n=== BILAN ===\n");
    std::printf("A %d  -  B %d  -  nulles %d   (%.1f%% pour A)\n",
                winA, winB, draws, games ? 100.0 * (winA + 0.5 * draws) / games : 0.0);
    std::printf("longueur moyenne : %.1f demi-actions\n", games ? (double)totalActions / games : 0.0);
    for (int i = 0; i < 4; ++i) if (reasons[i]) std::printf("fin par %-16s : %d\n", REASON[i], reasons[i]);
    auto rep = [&](const char* n, Agent& a) {
        if (!a.moves) return;
        std::printf("%s : %llu coups, %.1f ms moyen, %.1f ms MAX, %.0f noeuds/coup\n",
                    n, (unsigned long long)a.moves, a.sumMs / a.moves, a.maxMs,
                    (double)a.nodes / a.moves);
    };
    rep("A", A); rep("B", B);
    if (illegal) { std::printf("\n!! %d COUP(S) ILLEGAL(AUX) : BUG\n", illegal); return 1; }
    if (A.maxMs > 49 || B.maxMs > 49)
        std::printf("\n!! ATTENTION : depassement possible de la limite de 50 ms\n");
    return 0;
}
