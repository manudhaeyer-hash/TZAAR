#include "eval.hpp"

namespace tzaar {

EvalParams g_params;

// ---------------------------------------------------------------------------
//  Tables precalculees. L'evaluation etant appelee des centaines de milliers de
//  fois par tour, aucune division ne doit rester dans le chemin chaud.
//  Elles ne dependent que des parametres : reconstruites a chaque debut de
//  recherche (les parametres sont figes pendant une recherche).
// ---------------------------------------------------------------------------
namespace {

constexpr int MAXN = 64;
int g_group   [NUM_TYPES][MAXN];   // valeur totale d'un groupe de n piles
int g_marginal[NUM_TYPES][MAXN];   // valeur de la n-ieme pile du groupe
int g_threat  [NUM_TYPES][MAXN];   // gravite d'une menace sur ce groupe
bool g_tablesReady = false;

inline int curve(const EvalParams& P, int n) {
    if (n <= 15) return P.countCurve[n < 0 ? 0 : n];
    return P.countCurve[15] + P.countCurveTail * (n - 15);
}

int side_static(const Position& pos, const EvalParams& P, int s) {
    int v = 0;
    for (int t = 0; t < NUM_TYPES; ++t) v += g_group[t][pos.counts[s][t]];
    v += P.heightWeight * pos.tallBonus[s];
    v += P.pieceWeight  * pos.pieces[s];
    return v;
}

} // namespace

void eval_rebuild_tables() {
    const EvalParams& P = params();
    for (int t = 0; t < NUM_TYPES; ++t)
        for (int n = 0; n < MAXN; ++n) {
            g_group[t][n]    = P.typeValue[t] * curve(P, n) / 100;
            int marg         = curve(P, n) - curve(P, n > 0 ? n - 1 : 0);
            g_marginal[t][n] = P.typeValue[t] * marg / 100;
            g_threat[t][n]   = g_marginal[t][n] >> 5;
        }
    g_tablesReady = true;
}

namespace { struct TableInit { TableInit() { eval_rebuild_tables(); } } g_tableInit; }

// Terme de course : s'applique aux types dont l'un des deux camps approche de
// l'epuisement. Avoir le trait y vaut un demi-echange.
static int race_term(const Position& pos, const EvalParams& P, int side) {
    const int op = side ^ 1;
    int v = 0;
    for (int t = 0; t < NUM_TYPES; ++t) {
        const int a = pos.counts[side][t], b = pos.counts[op][t];
        if (a > P.raceThreshold && b > P.raceThreshold) continue;
        if      (a > b) v += P.raceEdge * (a - b);
        else if (a < b) v -= P.raceEdge * (b - a);
        else            v += P.raceTempo;   // egalite : le trait departage
    }
    return v;
}

// ---------------------------------------------------------------- statique --
int eval_static(const Position& pos, int side) {
    const EvalParams& P = params();
    return side_static(pos, P, side) - side_static(pos, P, side ^ 1)
         + race_term(pos, P, side);
}

// --------------------------------------------------------------- dynamique --
//  Un seul balayage du plateau donne d'un coup, pour les DEUX camps :
//   - le nombre de captures disponibles (mobilite, et surtout survie : ne pas
//     pouvoir capturer en phase 1 est une defaite) ;
//   - la gravite cumulee des menaces subies, ponderee par la rarete du type.
int eval_full(const Position& pos, int side) {
    const EvalParams& P = params();
    int score = eval_static(pos, side);
    if (!P.useDynamic) return score;

    int mob[2]    = { 0, 0 };
    int threat[2] = { 0, 0 };

    u64 all = pos.occ;
    while (all) {
        const int c  = pop_lsb(all);
        const int ow = pos.owner_[c];
        const int h  = pos.height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = pos.occ & GEO().ray[c][d];
            if (!b) continue;
            const int t = (d < 3) ? lsb(b) : msb(b);
            const int to = pos.owner_[t];
            if (to == ow || h < pos.height_[t]) continue;
            ++mob[ow];
            threat[to] += g_threat[pos.type_[t]][pos.counts[to][pos.type_[t]]];
        }
    }

    const int me = side, op = side ^ 1;
    score += P.mobilityWeight * (mob[me] - mob[op]);
    score -= (P.threatWeight * (threat[me] - threat[op])) >> 3;

    // Etouffement : sans capture disponible, l'adversaire perd des sa phase 1.
    // C'est une condition de victoire a part entiere, pas un simple bonus.
    if      (mob[op] == 0) score += P.zugzwangWeight * 3;
    else if (mob[op] == 1) score += P.zugzwangWeight;
    if      (mob[me] == 0) score -= P.zugzwangWeight * 3;
    else if (mob[me] == 1) score -= P.zugzwangWeight;

    return score;
}

// Evaluation paresseuse : si le terme O(1) sort deja largement de la fenetre,
// le terme dynamique ne changera pas la decision.
int eval(const Position& pos, int side, int alpha, int beta) {
    const EvalParams& P = params();
    const int s = eval_static(pos, side);
    if (!P.useDynamic) return s;
    if (s - P.lazyMargin >= beta)  return s;
    if (s + P.lazyMargin <= alpha) return s;
    return eval_full(pos, side);
}

// Balaye UNIQUEMENT les piles de `who` et renvoie sa mobilite ; accumule au
// passage la gravite des menaces qu'il fait peser sur l'autre camp.
static void scan_side(const Position& pos, int who, int& mobility, int& threatOnOther) {
    int mob = 0, thr = 0;
    const int other = who ^ 1;
    u64 mine = pos.occP[who];
    while (mine) {
        const int c = pop_lsb(mine);
        const int h = pos.height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = pos.occ & GEO().ray[c][d];
            if (!b) continue;
            const int t = (d < 3) ? lsb(b) : msb(b);
            if (pos.owner_[t] == who || h < pos.height_[t]) continue;
            ++mob;
            thr += g_threat[pos.type_[t]][pos.counts[other][pos.type_[t]]];
        }
    }
    mobility = mob; threatOnOther = thr;
}

int eval_leaf(const Position& pos, int side, const Move* myCaps, int nMyCaps,
              int alpha, int beta) {
    const EvalParams& P = params();
    const int s = eval_static(pos, side);
    if (!P.useDynamic) return s;
    if (s - P.lazyMargin >= beta)  return s;
    if (s + P.lazyMargin <= alpha) return s;

    const int op = side ^ 1;
    // Ma mobilite est gratuite : c'est le nombre de captures deja generees.
    const int mobMe = nMyCaps;
    int thrOnOpp = 0;
    for (int i = 0; i < nMyCaps; ++i) {
        const int t = move_to(myCaps[i]);
        thrOnOpp += g_threat[pos.type_[t]][pos.counts[op][pos.type_[t]]];
    }
    int mobOpp = 0, thrOnMe = 0;
    scan_side(pos, op, mobOpp, thrOnMe);

    int score = s;
    score += P.mobilityWeight * (mobMe - mobOpp);
    score -= (P.threatWeight * (thrOnMe - thrOnOpp)) >> 3;
    if      (mobOpp == 0) score += P.zugzwangWeight * 3;
    else if (mobOpp == 1) score += P.zugzwangWeight;
    if      (mobMe  == 0) score -= P.zugzwangWeight * 3;
    else if (mobMe  == 1) score -= P.zugzwangWeight;
    return score;
}

// ------------------------------------------------------------ ordonnancement --
int group_value   (int type, int n) { return g_group[type][n < MAXN ? n : MAXN - 1]; }
int piece_worth   (int type, int n) { return g_marginal[type][n < MAXN ? n : MAXN - 1]; }

int capture_value(const Position& pos, int victimCell) {
    const int ow = pos.owner_[victimCell], ty = pos.type_[victimCell];
    const int n  = pos.counts[ow][ty];
    if (n == 1) return 1 << 16;               // prend la derniere piece d'un type
    return piece_worth(ty, n) + 8 * pos.height_[victimCell];
}

int stack_value(const Position& pos, int from, int to) {
    const EvalParams& P = params();
    const int side = pos.owner_[from];
    const int hf = pos.height_[from], ht = pos.height_[to];
    const int gain = P.heightWeight * (height_bonus(hf + ht) - height_bonus(hf) - height_bonus(ht));
    return gain - piece_worth(pos.type_[to], pos.counts[side][pos.type_[to]]);
}

} // namespace tzaar
