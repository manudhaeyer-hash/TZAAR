// ==========================================================================
// TZAAR - solver mono-fichier genere par tools/bundle.py
// NE PAS EDITER : modifier les sources dans src/ puis relancer le bundler.
// ==========================================================================
#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("popcnt,bmi,bmi2")

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
//  Geometrie hexagonale + rayons precalcules.
//
//  IDEE CLE (perf) : les cases sont numerotees par (r croissant, puis q
//  croissant). Avec cet ordre, les 3 directions {(+1,0),(0,+1),(-1,+1)} font
//  TOUJOURS croitre l'indice, et leurs opposees le font toujours decroitre.
//  Trouver le premier bloqueur sur une ligne se reduit donc a :
//      bits = occ & ray[cell][dir];
//      cible = (dir < 3) ? lsb(bits) : msb(bits);
//  soit 2 instructions, sans boucle.
//
//  Le centre (0,0) n'existe pas et BLOQUE les lignes (cf. Board.isPathClear du
//  referee Java) : les rayons s'arretent donc juste avant lui.
// ============================================================================
#ifndef TZAAR_CORE_GEOMETRY_HPP
#define TZAAR_CORE_GEOMETRY_HPP

// ============================================================================
//  TZAAR solver - core types
//  Aucun include du projet ici : c'est la racine de la hierarchie.
// ============================================================================
#ifndef TZAAR_CORE_TYPES_HPP
#define TZAAR_CORE_TYPES_HPP


namespace tzaar {

using u8  = std::uint8_t;   using i8  = std::int8_t;
using u16 = std::uint16_t;  using i16 = std::int16_t;
using u32 = std::uint32_t;  using i32 = std::int32_t;
using u64 = std::uint64_t;  using i64 = std::int64_t;

// ---------------------------------------------------------------- plateau ---
// Hexagone de rayon 4 (61 cases) moins le centre  ->  60 cases.
constexpr int NUM_CELLS  = 60;
constexpr int NUM_DIRS   = 6;
constexpr int NUM_TYPES  = 3;
constexpr int BOARD_R    = 4;
// Une pile ne peut pas depasser les 30 pieces d'un joueur.
constexpr int MAX_HEIGHT = 30;

enum PieceType : u8 { TOTT = 0, TZARRA = 1, TZAAR = 2 };

// Le referee appelle le bot deux fois par tour.
//   phase 1 : capture OBLIGATOIRE (sinon defaite)
//   phase 2 : capture, empilement, ou WAIT
enum Phase : u8 { PHASE_CAPTURE = 1, PHASE_FREE = 2 };

// ------------------------------------------------------------------ coups ---
// 12 bits utiles : from(6) | to(6).  Deux sentinelles hors de cet espace.
using Move = u16;
constexpr Move MOVE_NONE = 0xFFFF;
constexpr Move MOVE_WAIT = 0xFFFE;

constexpr Move mk_move(int from, int to) { return (Move)((from << 6) | to); }
constexpr int  move_from(Move m)         { return (m >> 6) & 63; }
constexpr int  move_to  (Move m)         { return m & 63; }
constexpr bool is_wait  (Move m)         { return m == MOVE_WAIT; }
constexpr bool is_real   (Move m)        { return m < 0xF000; }

// 30 piles x 6 directions + WAIT = 181 au maximum absolu.
constexpr int MAX_MOVES = 200;
constexpr int MAX_PLY   = 160;   // la partie est bornee : ~120 demi-coups

// ------------------------------------------------------------------ scores --
constexpr int VALUE_MATE     = 1000000;
constexpr int VALUE_INF      = 2000000;
constexpr int VALUE_MATE_MIN = VALUE_MATE - MAX_PLY;   // seuil "mat trouve"

constexpr bool is_mate_score(int v) { return v >= VALUE_MATE_MIN || v <= -VALUE_MATE_MIN; }

// --------------------------------------------------------------- bit utils --
inline int  lsb  (u64 b) { return __builtin_ctzll(b); }
inline int  msb  (u64 b) { return 63 - __builtin_clzll(b); }
inline int  popcnt(u64 b){ return __builtin_popcountll(b); }
inline int  pop_lsb(u64& b) { int i = lsb(b); b &= b - 1; return i; }

} // namespace tzaar
#endif


namespace tzaar {

// Ordre impose : indices 0..2 = directions "croissantes", 3..5 = opposees.
struct HexDir { i8 dq, dr; };
constexpr HexDir DIRS[NUM_DIRS] = {
    { +1,  0 }, {  0, +1 }, { -1, +1 },   // croissantes
    { -1,  0 }, {  0, -1 }, { +1, -1 },   // decroissantes
};
constexpr int opposite_dir(int d) { return (d + 3) % 6; }

struct Geometry {
    i8  q[NUM_CELLS];              // coordonnee cube q de chaque case
    i8  r[NUM_CELLS];              // coordonnee cube r
    i8  ring[NUM_CELLS];           // anneau 1..4
    i16 index[9][9];               // [x][y] -> case, -1 si hors plateau (x=q+4)
    u64 ray[NUM_CELLS][NUM_DIRS];  // cases alignees, self exclu, stoppe au centre
    u64 all;                       // masque des 60 cases

    Geometry();
};

// Objet global : un `static` local couterait une garde d'initialisation
// thread-safe a CHAQUE appel, or GEO() est dans la boucle la plus chaude.
extern const Geometry g_geometry;
inline const Geometry& GEO() { return g_geometry; }

// Conversion protocole CodinGame <-> index interne. (x,y) = (q+4, r+4).
inline int cell_from_xy(int x, int y) {
    if (x < 0 || x > 8 || y < 0 || y > 8) return -1;
    return GEO().index[x][y];
}
inline int cell_x(int c) { return GEO().q[c] + BOARD_R; }
inline int cell_y(int c) { return GEO().r[c] + BOARD_R; }

// Premiere case occupee en partant de `c` dans la direction `d`. -1 si aucune.
inline int first_blocker(u64 occ, int c, int d) {
    u64 b = occ & GEO().ray[c][d];
    if (!b) return -1;
    return (d < 3) ? lsb(b) : msb(b);
}

} // namespace tzaar
#endif


namespace tzaar {

Geometry::Geometry() {
    for (int x = 0; x < 9; ++x)
        for (int y = 0; y < 9; ++y) index[x][y] = -1;

    // --- numerotation : r croissant, puis q croissant (le centre est exclu) ---
    int n = 0;
    for (int rr = -BOARD_R; rr <= BOARD_R; ++rr) {
        int qmin = (-BOARD_R > -BOARD_R - rr) ? -BOARD_R : -BOARD_R - rr;
        int qmax = ( BOARD_R <  BOARD_R - rr) ?  BOARD_R :  BOARD_R - rr;
        for (int qq = qmin; qq <= qmax; ++qq) {
            if (qq == 0 && rr == 0) continue;          // trou central
            q[n] = (i8)qq;
            r[n] = (i8)rr;
            int ss = -qq - rr;
            int a = qq < 0 ? -qq : qq, b = rr < 0 ? -rr : rr, cc = ss < 0 ? -ss : ss;
            ring[n] = (i8)((a + b + cc) / 2);
            index[qq + BOARD_R][rr + BOARD_R] = (i16)n;
            ++n;
        }
    }
    // n == NUM_CELLS == 60

    all = 0;
    for (int i = 0; i < NUM_CELLS; ++i) all |= 1ull << i;

    // --- rayons : on avance case par case, on s'arrete au bord ou au centre ---
    for (int c = 0; c < NUM_CELLS; ++c) {
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 m = 0;
            int cq = q[c], cr = r[c];
            for (;;) {
                cq += DIRS[d].dq;
                cr += DIRS[d].dr;
                if (cq < -BOARD_R || cq > BOARD_R || cr < -BOARD_R || cr > BOARD_R) break;
                int ss = -cq - cr;
                if (ss < -BOARD_R || ss > BOARD_R) break;
                if (cq == 0 && cr == 0) break;         // le centre bloque la ligne
                m |= 1ull << index[cq + BOARD_R][cr + BOARD_R];
            }
            ray[c][d] = m;
        }
    }
}

const Geometry g_geometry;

} // namespace tzaar

// ============================================================================
//  Cles Zobrist. Une pile est identifiee par (case, proprietaire, type, hauteur)
//  car la hauteur fait partie de l'etat (elle change lors des empilements).
// ============================================================================
#ifndef TZAAR_CORE_ZOBRIST_HPP
#define TZAAR_CORE_ZOBRIST_HPP


namespace tzaar {

struct Zobrist {
    u64 stack[NUM_CELLS][2][NUM_TYPES][MAX_HEIGHT + 1];
    u64 side[2];
    u64 phase[3];
    Zobrist();
};

extern const Zobrist g_zobrist;
inline const Zobrist& ZOB() { return g_zobrist; }

inline u64 zkey(int cell, int owner, int type, int height) {
    return ZOB().stack[cell][owner][type][height];
}

} // namespace tzaar
#endif


namespace tzaar {

namespace {
// splitmix64 : deterministe, donc parties reproductibles et bugs rejouables.
struct SplitMix { u64 s;
    u64 next() {
        u64 z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
};
}

Zobrist::Zobrist() {
    SplitMix rng{ 0xC0FFEE123456789Aull };  // graine fixe -> parties reproductibles
    for (int c = 0; c < NUM_CELLS; ++c)
        for (int o = 0; o < 2; ++o)
            for (int t = 0; t < NUM_TYPES; ++t)
                for (int h = 0; h <= MAX_HEIGHT; ++h)
                    stack[c][o][t][h] = rng.next();
    side[0] = rng.next(); side[1] = rng.next();
    phase[0] = 0; phase[1] = rng.next(); phase[2] = rng.next();
}

const Zobrist g_zobrist;

} // namespace tzaar

// ============================================================================
//  Etat de jeu. Tout ce qui sert a l'evaluation est maintenu INCREMENTALEMENT
//  (compteurs par type, pieces totales, bonus de hauteur) : eval() est O(1)
//  hors terme de mobilite.
//
//  Regles encodees ici, verifiees contre Board.java / Referee.java :
//   - on ne se deplace QUE sur une case occupee (jamais sur du vide) ;
//   - la ligne doit etre libre, et le trou central bloque ;
//   - capture : hauteur source >= hauteur cible ; la source remplace la cible,
//     SANS gagner de hauteur ;
//   - empilement (pile alliee) : hauteurs additionnees, le type devient celui
//     de la pile qui arrive -> enterrer sa derniere piece d'un type FAIT PERDRE.
// ============================================================================
#ifndef TZAAR_CORE_POSITION_HPP
#define TZAAR_CORE_POSITION_HPP


namespace tzaar {

// Bonus de hauteur plafonne : au-dela, empiler n'apporte plus rien.
constexpr int HEIGHT_BONUS_CAP = 5;
inline int height_bonus(int h) { int b = h - 1; return b > HEIGHT_BONUS_CAP ? HEIGHT_BONUS_CAP : b; }

struct Undo {
    Move move;
    u8   fromHeight;   // hauteur de la pile qui bouge (avant coup)
    u8   fromType;
    u8   toOwner, toType, toHeight;
    u64  hash;
};

struct Position {
    u64 occ;
    u64 occP[2];
    u8  owner_ [NUM_CELLS];
    u8  type_  [NUM_CELLS];
    u8  height_[NUM_CELLS];

    int counts   [2][NUM_TYPES]; // nb de PILES dont le sommet est de ce type
    int stacks   [2];            // nb de piles
    int pieces   [2];            // somme des hauteurs (pieces physiques)
    int tallBonus[2];            // somme des height_bonus()
    u64 hash;

    // ---------------------------------------------------------- construction
    void clear();
    void put(int cell, int owner, int type, int height);   // pose sans hash
    void refresh();                                        // recalcule hash + derives
    void init_start();                                     // position initiale officielle

    // -------------------------------------------------------------- lecture
    bool empty_at(int c) const { return !((occ >> c) & 1); }
    int  missing_type(int side) const {
        for (int t = 0; t < NUM_TYPES; ++t) if (counts[side][t] == 0) return t;
        return -1;
    }
    u64  key(int side, int phase) const { return hash ^ ZOB().side[side] ^ ZOB().phase[phase]; }

    // ------------------------------------------------------- generation ----
    int  gen_moves   (Move* out, int side, int phase) const;  // legaux complets
    int  gen_captures(Move* out, int side) const;
    // Un seul balayage produisant les deux listes : les captures d'un cote, les
    // empilements de l'autre. La recherche ne note et ne trie les empilements
    // que si aucune capture n'a provoque de coupure -> gros gain.
    void gen_split(Move* caps, int& nCaps, Move* quiets, int& nQuiets, int side) const;
    bool has_capture (int side) const;
    // Capture qui retire a l'adversaire sa DERNIERE pile d'un type : gain
    // immediat et prouve. Sortie rapide (aucun scan si aucun type critique).
    Move winning_capture(int side) const;
    bool is_legal    (Move m, int side, int phase) const;     // validation stricte

    // ------------------------------------------------------------ mutation --
    void do_move  (Move m, Undo& u);
    void undo_move(const Undo& u);

private:
    void add_stack(int cell, int owner, int type, int height);
    void del_stack(int cell);
};

} // namespace tzaar
#endif


namespace tzaar {

void Position::clear() {
    occ = occP[0] = occP[1] = 0;
    std::memset(owner_, 0, sizeof owner_);
    std::memset(type_,  0, sizeof type_);
    std::memset(height_,0, sizeof height_);
    for (int p = 0; p < 2; ++p) {
        stacks[p] = pieces[p] = tallBonus[p] = 0;
        for (int t = 0; t < NUM_TYPES; ++t) counts[p][t] = 0;
    }
    hash = 0;
}

void Position::add_stack(int cell, int owner, int type, int height) {
    u64 bit = 1ull << cell;
    occ |= bit; occP[owner] |= bit;
    owner_[cell] = (u8)owner; type_[cell] = (u8)type; height_[cell] = (u8)height;
    counts[owner][type]++; stacks[owner]++; pieces[owner] += height;
    tallBonus[owner] += height_bonus(height);
    hash ^= zkey(cell, owner, type, height);
}

void Position::del_stack(int cell) {
    int owner = owner_[cell], type = type_[cell], height = height_[cell];
    u64 bit = 1ull << cell;
    occ &= ~bit; occP[owner] &= ~bit;
    counts[owner][type]--; stacks[owner]--; pieces[owner] -= height;
    tallBonus[owner] -= height_bonus(height);
    hash ^= zkey(cell, owner, type, height);
}

void Position::put(int cell, int owner, int type, int height) {
    if (!empty_at(cell)) del_stack(cell);
    add_stack(cell, owner, type, height);
}

void Position::refresh() {
    // Recalcule integralement les derives a partir de occ/owner_/type_/height_.
    u64 o = occ;
    for (int p = 0; p < 2; ++p) {
        stacks[p] = pieces[p] = tallBonus[p] = 0;
        occP[p] = 0;
        for (int t = 0; t < NUM_TYPES; ++t) counts[p][t] = 0;
    }
    hash = 0;
    while (o) {
        int c = pop_lsb(o);
        int ow = owner_[c], ty = type_[c], h = height_[c];
        occP[ow] |= 1ull << c;
        counts[ow][ty]++; stacks[ow]++; pieces[ow] += h;
        tallBonus[ow] += height_bonus(h);
        hash ^= zkey(c, ow, ty, h);
    }
}

// ---------------------------------------------------------------------------
//  Position de depart : reproduit exactement Board.initNonRandom() du referee.
//  getRing(radius) part de dir[4]*radius puis parcourt les 6 cotes.
// ---------------------------------------------------------------------------
namespace {
struct Cube { int q, r; };
constexpr Cube RING_DIRS[6] = { {1,0}, {0,1}, {-1,1}, {-1,0}, {0,-1}, {1,-1} };

void ring_cells(int radius, int* out) {
    Cube h{ RING_DIRS[4].q * radius, RING_DIRS[4].r * radius };
    int n = 0;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < radius; ++j) {
            out[n++] = GEO().index[h.q + BOARD_R][h.r + BOARD_R];
            h.q += RING_DIRS[i].q; h.r += RING_DIRS[i].r;
        }
}
}

void Position::init_start() {
    clear();
    int buf[24];
    ring_cells(1, buf); for (int i = 0; i < 6;  ++i) add_stack(buf[i], i % 2,       TOTT,   1);
    ring_cells(2, buf); for (int i = 0; i < 12; ++i) add_stack(buf[i], (i / 2) % 2, TZAAR,  1);
    ring_cells(3, buf); for (int i = 0; i < 18; ++i) add_stack(buf[i], (i / 3) % 2, TZARRA, 1);
    ring_cells(4, buf); for (int i = 0; i < 24; ++i) add_stack(buf[i], (i / 4) % 2, TOTT,   1);
}

// ---------------------------------------------------------------- movegen ---
int Position::gen_moves(Move* out, int side, int phase) const {
    int n = 0;
    u64 mine = occP[side];
    while (mine) {
        int c = pop_lsb(mine);
        int h = height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = occ & GEO().ray[c][d];
            if (!b) continue;
            int t = (d < 3) ? lsb(b) : msb(b);
            if (owner_[t] == side) {
                if (phase == PHASE_FREE) out[n++] = mk_move(c, t);
            } else if (h >= height_[t]) {
                out[n++] = mk_move(c, t);
            }
        }
    }
    if (phase == PHASE_FREE) out[n++] = MOVE_WAIT;   // toujours legal en phase 2
    return n;
}

int Position::gen_captures(Move* out, int side) const {
    int n = 0;
    u64 mine = occP[side];
    while (mine) {
        int c = pop_lsb(mine);
        int h = height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = occ & GEO().ray[c][d];
            if (!b) continue;
            int t = (d < 3) ? lsb(b) : msb(b);
            if (owner_[t] != side && h >= height_[t]) out[n++] = mk_move(c, t);
        }
    }
    return n;
}

void Position::gen_split(Move* caps, int& nCaps, Move* quiets, int& nQuiets, int side) const {
    int nc = 0, nq = 0;
    u64 mine = occP[side];
    while (mine) {
        const int c = pop_lsb(mine);
        const int h = height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = occ & GEO().ray[c][d];
            if (!b) continue;
            const int t = (d < 3) ? lsb(b) : msb(b);
            if (owner_[t] == side)      quiets[nq++] = mk_move(c, t);
            else if (h >= height_[t])   caps  [nc++] = mk_move(c, t);
        }
    }
    nCaps = nc; nQuiets = nq;
}

bool Position::has_capture(int side) const {
    u64 mine = occP[side];
    while (mine) {
        int c = pop_lsb(mine);
        int h = height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = occ & GEO().ray[c][d];
            if (!b) continue;
            int t = (d < 3) ? lsb(b) : msb(b);
            if (owner_[t] != side && h >= height_[t]) return true;
        }
    }
    return false;
}

Move Position::winning_capture(int side) const {
    const int opp = side ^ 1;
    int crit = 0;
    for (int t = 0; t < NUM_TYPES; ++t) if (counts[opp][t] == 1) crit |= 1 << t;
    if (!crit) return MOVE_NONE;
    u64 mine = occP[side];
    while (mine) {
        int c = pop_lsb(mine);
        int h = height_[c];
        for (int d = 0; d < NUM_DIRS; ++d) {
            u64 b = occ & GEO().ray[c][d];
            if (!b) continue;
            int t = (d < 3) ? lsb(b) : msb(b);
            if (owner_[t] == side || h < height_[t]) continue;
            if ((crit >> type_[t]) & 1) return mk_move(c, t);
        }
    }
    return MOVE_NONE;
}

bool Position::is_legal(Move m, int side, int phase) const {
    if (is_wait(m)) return phase == PHASE_FREE;
    if (!is_real(m)) return false;
    int from = move_from(m), to = move_to(m);
    if (from >= NUM_CELLS || to >= NUM_CELLS || from == to) return false;
    if (empty_at(from) || empty_at(to)) return false;
    if (owner_[from] != side) return false;
    // La cible doit etre le premier bloqueur sur une des 6 lignes.
    bool reachable = false;
    for (int d = 0; d < NUM_DIRS && !reachable; ++d) {
        if (!((GEO().ray[from][d] >> to) & 1)) continue;
        u64 b = occ & GEO().ray[from][d];
        reachable = (((d < 3) ? lsb(b) : msb(b)) == to);
    }
    if (!reachable) return false;
    if (owner_[to] == side) return phase == PHASE_FREE;      // empilement
    return height_[from] >= height_[to];                     // capture
}

// --------------------------------------------------------------- mutation ---
void Position::do_move(Move m, Undo& u) {
    u.move = m;
    u.hash = hash;
    if (is_wait(m)) { u.fromHeight = 0; u.toHeight = 0; return; }

    int from = move_from(m), to = move_to(m);
    int side = owner_[from];
    u.fromHeight = height_[from];
    u.fromType   = type_[from];
    u.toOwner    = owner_[to];
    u.toType     = type_[to];
    u.toHeight   = height_[to];

    int newHeight = (u.toOwner == side) ? (u.fromHeight + u.toHeight) : u.fromHeight;

    del_stack(to);
    del_stack(from);
    add_stack(to, side, u.fromType, newHeight);
}

void Position::undo_move(const Undo& u) {
    if (is_wait(u.move)) { hash = u.hash; return; }
    int from = move_from(u.move), to = move_to(u.move);
    int side = owner_[to];
    del_stack(to);
    add_stack(from, side, u.fromType, u.fromHeight);
    add_stack(to, u.toOwner, u.toType, u.toHeight);
    hash = u.hash;
}

} // namespace tzaar

// ============================================================================
//  Evaluation. Deux etages :
//   1. eval_static  : O(1), n'utilise que les compteurs incrementaux.
//   2. eval_dynamic : un seul balayage du plateau (<= 60 x 6 sondes) qui produit
//      d'un coup la mobilite ET les menaces des DEUX camps.
//  eval() enchaine les deux avec un court-circuit "lazy" pilote par alpha/beta.
// ============================================================================
#ifndef TZAAR_ENGINE_EVAL_HPP
#define TZAAR_ENGINE_EVAL_HPP

// ============================================================================
//  TOUS les poids de l'evaluation sont ici, dans une seule struct modifiable a
//  chaud. C'est le point d'entree du tuning : l'arene (tools/arena) peut faire
//  s'affronter deux jeux de parametres sans recompiler la recherche.
// ============================================================================
#ifndef TZAAR_ENGINE_EVAL_PARAMS_HPP
#define TZAAR_ENGINE_EVAL_PARAMS_HPP


namespace tzaar {

struct EvalParams {
    // ---- valeur relative des trois types (TOTT / TZARRA / TZAAR) -----------
    // Il y a 15 TOTT, 9 TZARRA et 6 TZAAR par joueur : les TZAAR sont les plus
    // critiques parce qu'ils s'epuisent le plus vite.
    int typeValue[NUM_TYPES] = { 100, 118, 140 };

    // ---- courbe de rarete --------------------------------------------------
    // Valeur cumulee d'un groupe de N piles d'un meme type. Concave : passer de
    // 1 a 2 vaut enormement, passer de 10 a 11 presque rien. C'est ce qui
    // encode "perdre tous ses X = defaite" de facon continue.
    // Index 0 est un cas perdu gere en amont (score de mat).
    int countCurve[16] = { 0, 100, 196, 276, 340, 392, 434, 468, 494,
                           514, 530, 542, 552, 560, 566, 570 };
    int countCurveTail = 4;     // increment lineaire au-dela de l'index 15

    // ---- hauteur -----------------------------------------------------------
    // Une pile haute capture tout et ne peut etre prise que par plus haut
    // qu'elle, mais elle immobilise des pieces : gain plafonne.
    int heightWeight = 26;      // applique a tallBonus (somme des min(h-1,CAP))
    int pieceWeight  = 3;       // applique aux pieces physiques totales

    // ---- course aux pieces rares (DESACTIVE par defaut) --------------------
    // Constat : une evaluation a somme nulle juge un echange 1 contre 1 neutre,
    // et les deux moteurs echangent donc leurs TZAAR de 6-6 a 1-1 en dix coups.
    // Ce terme rend l'asymetrie du trait visible. Il n'est PAS a somme nulle,
    // mais reste correct en negamax : l'evaluation n'est appelee qu'en frontiere
    // de tour, toujours du point de vue du camp au trait.
    //
    // MESURE : 40 parties a 20 ms, raceTempo=260 raceEdge=180 -> 47,5 %.
    // Aucun gain mesurable, donc neutralise par defaut. Le levier reste en
    // place : c'est la premiere piste a reprendre avec un budget de parties
    // plus large ou d'autres poids.
    int raceThreshold = 3;    // "type presque epuise" : effectif <= ce seuil
    int raceTempo     = 0;    // a effectif egal, avoir le trait
    int raceEdge      = 0;    // par exemplaire d'avance sur un type rare

    // ---- termes dynamiques (scan du plateau, actives a la demande) ---------
    int mobilityWeight = 7;     // par capture disponible
    int threatWeight   = 14;    // par pile menacee, pondere par la rarete du type
    int zugzwangWeight = 260;   // bonus si l'adversaire n'a presque plus de captures

    // Marge de l'evaluation paresseuse : si le terme O(1) sort deja de la
    // fenetre alpha/beta de plus que ca, on n'evalue pas le terme dynamique.
    int lazyMargin = 420;
    bool useDynamic = true;

    // ---- recherche ---------------------------------------------------------
    int  aspirationWindow = 60;
    int  lmrMinDepth      = 3;
    int  lmrMinMove       = 4;
    bool useLMR           = true;
    bool useTT            = true;

    // Late Move Pruning : la phase 2 offre ~145 coups dont l'immense majorite
    // sont des empilements sans interet. On plafonne le nombre d'empilements
    // reellement explores en fonction de la profondeur restante.
    int  lmpBase   = 4;
    int  lmpFactor = 3;      // quiets explores = lmpBase + lmpFactor * depth
    bool useLMP    = true;

    // Meme idee sur les captures : la phase 1 en propose ~43, dont beaucoup
    // sont sans objet (prendre un pion isole avec sa plus grosse pile).
    // 0 = desactive.
    int  capBase   = 0;
    int  capFactor = 0;      // captures explorees = capBase + capFactor * depth
};

// Jeu de parametres courant du processus (modifiable pour le tuning).
extern EvalParams g_params;
inline EvalParams& params() { return g_params; }

} // namespace tzaar
#endif


namespace tzaar {

// Score du point de vue de `side`. Toujours appele sur une position non finie.
int eval_static (const Position& pos, int side);
int eval_full   (const Position& pos, int side);
int eval        (const Position& pos, int side, int alpha, int beta);

// Version feuille : la recherche vient de generer les captures du camp au
// trait, donc sa mobilite et ses menaces sont deja connues. Seul le camp
// adverse reste a balayer -> moitie moins de sondes que eval_full.
int eval_leaf(const Position& pos, int side, const Move* myCaps, int nMyCaps,
              int alpha, int beta);

// Reconstruit les tables derivees des parametres. A appeler apres toute
// modification de params().
void eval_rebuild_tables();

// --- primitives d'ordonnancement (toutes en O(1), sans division) ------------
int group_value (int type, int n);   // valeur totale d'un groupe de n piles
int piece_worth (int type, int n);   // valeur de la n-ieme pile : explose quand n est petit
int capture_value(const Position& pos, int victimCell);
int stack_value  (const Position& pos, int from, int to);

} // namespace tzaar
#endif


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

// ============================================================================
//  Table de transposition. Entree de 16 octets, remplacement "profondeur ou
//  vieillesse". Taille en puissance de 2 -> indexation par masque.
// ============================================================================
#ifndef TZAAR_ENGINE_TT_HPP
#define TZAAR_ENGINE_TT_HPP


namespace tzaar {

enum TTFlag : u8 { TT_EMPTY = 0, TT_EXACT = 1, TT_LOWER = 2, TT_UPPER = 3 };

struct TTEntry {
    u64  key;      // cle complete : pas de collision silencieuse
    i32  score;
    Move move;
    i8   depth;
    u8   flag : 2;
    u8   age  : 6;
};
static_assert(sizeof(TTEntry) == 16, "TTEntry doit tenir en 16 octets");

class TT {
public:
    void resize_mb(size_t mb);
    void clear();
    void new_generation() { age_ = (age_ + 1) & 63; }

    bool probe(u64 key, TTEntry& out) const {
        const TTEntry& e = tab_[key & mask_];
        if (e.flag != TT_EMPTY && e.key == key) { out = e; return true; }
        return false;
    }
    void store(u64 key, int score, Move move, int depth, TTFlag flag) {
        TTEntry& e = tab_[key & mask_];
        bool replace = (e.flag == TT_EMPTY) || (e.key == key && depth >= e.depth)
                    || (e.age != age_) || (depth >= e.depth + 2);
        if (!replace) return;
        e.key = key; e.score = score; e.move = move;
        e.depth = (i8)depth; e.flag = flag; e.age = age_;
    }
    size_t entries() const { return tab_.size(); }

private:
    std::vector<TTEntry> tab_;
    u64    mask_ = 0;
    u8     age_  = 0;
};

} // namespace tzaar
#endif


namespace tzaar {

void TT::resize_mb(size_t mb) {
    size_t want = (mb * 1024 * 1024) / sizeof(TTEntry);
    size_t n = 1;
    while (n * 2 <= want) n *= 2;
    if (n < 1024) n = 1024;
    tab_.assign(n, TTEntry{});
    mask_ = n - 1;
    age_ = 0;
}

void TT::clear() {
    for (auto& e : tab_) e = TTEntry{};
    age_ = 0;
}

} // namespace tzaar

// ============================================================================
//  Recherche alpha-beta.
//
//  STRUCTURE DE L'ARBRE - specifique a TZAAR :
//  un "coup" du referee = une demi-action. Le tour d'un joueur vaut deux
//  noeuds consecutifs du MEME camp (phase 1 puis phase 2), puis on passe a
//  l'adversaire. La negation negamax n'a donc lieu qu'a la transition
//  phase2 -> phase1. La fenetre alpha/beta est transmise telle quelle entre
//  phase 1 et phase 2 (meme joueur = deux noeuds maximisants d'affilee, ce que
//  l'alpha-beta gere sans probleme).
//
//  L'evaluation n'a lieu QUE sur une frontiere de tour (phase 1), ce qui
//  supprime tout biais pair/impair et rend une quiescence separee inutile.
//
//  PROPRIETE UTILE : la phase 1 impose une capture, donc chaque tour retire au
//  moins une pile du plateau. La partie est finie (~60 tours max), sans
//  repetition ni nulle par cycle : pas de detection de repetition necessaire,
//  et la recherche atteint naturellement des noeuds terminaux exacts en finale.
// ============================================================================
#ifndef TZAAR_ENGINE_SEARCH_HPP
#define TZAAR_ENGINE_SEARCH_HPP

#ifndef TZAAR_ENGINE_TIMER_HPP
#define TZAAR_ENGINE_TIMER_HPP


namespace tzaar {

class Timer {
public:
    void start() { t0_ = clock::now(); }
    i64 us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - t0_).count();
    }
    double ms() const { return us() / 1000.0; }
private:
    using clock = std::chrono::steady_clock;
    clock::time_point t0_ = clock::now();
};

} // namespace tzaar
#endif


namespace tzaar {

struct SearchLimits {
    int  maxDepth   = 64;      // en demi-actions
    i64  timeUs     = 40000;   // budget dur
    i64  softTimeUs = 0;       // 0 = 60% de timeUs : on n'entame pas une iteration
};

struct SearchStats {
    u64  nodes      = 0;
    u64  leaves     = 0;
    u64  ttHits     = 0;
    u64  betaCuts   = 0;
    u64  firstCuts  = 0;       // coupures sur le 1er coup -> qualite de l'ordre
    int  depth      = 0;
    int  seldepth   = 0;
    int  score      = 0;
    Move best       = MOVE_NONE;
    double elapsedMs = 0;
    std::string pv;
    bool  proven    = false;   // score de mat : issue prouvee
};

class Searcher {
public:
    Searcher();
    void set_hash_mb(size_t mb);
    void new_game();

    // firstActionOfGame : au tout premier coup, le joueur 0 ne joue QUE la
    // phase 1 puis la main passe (cf. Referee.gameTurn).
    Move think(const Position& pos, int side, int phase,
               bool firstActionOfGame, const SearchLimits& lim, SearchStats& st);

    TT& tt() { return tt_; }

private:
    int negamax(int depth, int alpha, int beta, int side, int phase, int ply);
    int  score_move(Move m, int side, Move ttMove, int ply) const;
    u64  key_of(int side, int phase) const;
    // Valeur marginale d'une pile de chaque (camp, type) dans la position
    // courante. Recalculee une fois par noeud au lieu d'une fois par coup :
    // evite ~145 divisions par noeud.
    void refresh_worth();
    int  worth_[2][NUM_TYPES];
    void update_pv(int ply, Move m);

    Position pos_;
    TT       tt_;
    Timer    timer_;
    i64      budgetUs_ = 0;
    bool     stop_     = false;
    SearchStats* st_   = nullptr;

    Move killers_[MAX_PLY][2];
    int  history_[2][NUM_CELLS][NUM_CELLS];

    Move pv_[MAX_PLY][MAX_PLY];
    int  pvLen_[MAX_PLY];
};

std::string move_to_string(Move m);

} // namespace tzaar
#endif

// ============================================================================
//  Outils de debug. Tout passe par stderr : sur CodinGame stderr est affiche
//  dans la console sans polluer le protocole (stdout).
//
//  Compiler avec -DTZAAR_TRACE=1 pour activer TRACE().
//  Compiler avec -DTZAAR_ASSERT=1 pour activer les invariants (couteux).
// ============================================================================
#ifndef TZAAR_DEBUG_TRACE_HPP
#define TZAAR_DEBUG_TRACE_HPP


namespace tzaar {

// Rendu ASCII du plateau hexagonal (lignes r = -4..4, indentees).
std::string board_to_string(const Position& pos, int highlightFrom = -1, int highlightTo = -1);
// Dump compact rejouable : une ligne par pile, format identique a l'entree CG.
std::string position_to_input(const Position& pos);
// Relit un dump produit par position_to_input().
bool position_from_input(Position& pos, const std::string& text);

void trace_printf(const char* fmt, ...);

// Verifie que les compteurs incrementaux collent au plateau reel.
bool check_invariants(const Position& pos, std::string* why = nullptr);

} // namespace tzaar

#if defined(TZAAR_TRACE) && TZAAR_TRACE
  #define TRACE(...) ::tzaar::trace_printf(__VA_ARGS__)
#else
  #define TRACE(...) ((void)0)
#endif

#if defined(TZAAR_ASSERT) && TZAAR_ASSERT
  #define TZ_ASSERT(cond, msg) do { if (!(cond)) { \
        std::fprintf(stderr, "ASSERT %s:%d: %s (%s)\n", __FILE__, __LINE__, msg, #cond); \
        std::abort(); } } while (0)
#else
  #define TZ_ASSERT(cond, msg) ((void)0)
#endif

#endif


namespace tzaar {

// ---- bornes d'ordonnancement (decroissantes) -------------------------------
static constexpr int ORD_TT       = 1 << 30;
static constexpr int ORD_WINNING  = 1 << 28;   // capture la derniere piece d'un type
static constexpr int ORD_CAPTURE  = 1 << 20;
static constexpr int ORD_KILLER   = 1 << 19;
static constexpr int ORD_QUIET    = 1 << 10;

std::string move_to_string(Move m) {
    if (is_wait(m))  return "WAIT";
    if (!is_real(m)) return "(none)";
    char buf[40];
    std::snprintf(buf, sizeof buf, "MOVE %d %d %d %d",
                  cell_x(move_from(m)), cell_y(move_from(m)),
                  cell_x(move_to(m)),   cell_y(move_to(m)));
    return buf;
}

// Cle TT : la position seule ne suffit pas, il faut le camp au trait ET la
// phase (les coups legaux en dependent).
inline u64 Searcher::key_of(int side, int phase) const { return pos_.key(side, phase); }

Searcher::Searcher() {
    tt_.resize_mb(64);
    new_game();
}

void Searcher::set_hash_mb(size_t mb) { tt_.resize_mb(mb); }

void Searcher::new_game() {
    tt_.clear();
    std::memset(killers_, 0xFF, sizeof killers_);
    std::memset(history_, 0, sizeof history_);
}

// ---------------------------------------------------------------------------
void Searcher::refresh_worth() {
    for (int o = 0; o < 2; ++o)
        for (int t = 0; t < NUM_TYPES; ++t)
            worth_[o][t] = piece_worth(t, pos_.counts[o][t]);
}

// Les coups sont classes en 4 paliers strictement disjoints :
//   TT > captures > WAIT/killers > empilements.
// Les paliers etant ordonnes, des qu'on atteint un empilement tous les
// suivants en sont aussi : la boucle peut sortir net (voir LMP).
int Searcher::score_move(Move m, int side, Move ttMove, int ply) const {
    if (m == ttMove) return ORD_TT;
    // WAIT en tete des coups tranquilles : c'est l'option "ne rien risquer",
    // souvent correcte en phase 2, et elle ne doit jamais etre elaguee.
    if (is_wait(m))  return ORD_KILLER + 2;

    const int from = move_from(m), to = move_to(m);
    const int tt   = pos_.type_[to];

    if (pos_.owner_[to] != side)
        return ORD_CAPTURE + worth_[side ^ 1][tt] + 8 * pos_.height_[to];

    if (m == killers_[ply][0]) return ORD_KILLER + 1;
    if (m == killers_[ply][1]) return ORD_KILLER;

    // Empilement : gain de hauteur plafonne moins la valeur du type enterre.
    const int hf = pos_.height_[from], ht = pos_.height_[to];
    int v = params().heightWeight * (height_bonus(hf + ht) - height_bonus(hf) - height_bonus(ht))
          - worth_[side][tt];
    if (v < -4000) v = -4000; else if (v > 4000) v = 4000;
    int h = history_[side][from][to];
    if (h > (1 << 15)) h = 1 << 15;
    return ORD_QUIET + v * 8 + h;
}

void Searcher::update_pv(int ply, Move m) {
    pv_[ply][0] = m;
    int n = pvLen_[ply + 1];
    for (int i = 0; i < n; ++i) pv_[ply][i + 1] = pv_[ply + 1][i];
    pvLen_[ply] = n + 1;
}

// ---------------------------------------------------------------------------
int Searcher::negamax(int depth, int alpha, int beta, int side, int phase, int ply) {
    st_->nodes++;
    if (ply > st_->seldepth) st_->seldepth = ply;
    pvLen_[ply] = 0;

    // Controle du temps tous les 512 noeuds : a ~1,5 M noeuds/s cela borne le
    // depassement a ~0,3 ms, negligeable devant la marge de securite.
    if ((st_->nodes & 511) == 0 && timer_.us() >= budgetUs_) { stop_ = true; return 0; }
    if (ply >= MAX_PLY - 2) return eval(pos_, side, alpha, beta);

    const int opp = side ^ 1;

    // ---- conditions de fin, dans l'ordre exact du referee -------------------
    // 1) le joueur au trait a perdu tous ses exemplaires d'un type
    if (pos_.missing_type(side) >= 0) return -VALUE_MATE + ply;
    // 2) idem pour l'adversaire : irreversible (on ne deterre jamais une piece)
    if (pos_.missing_type(opp) >= 0)  return  VALUE_MATE - ply;

    // ---- gain immediat prouve ----------------------------------------------
    // Prendre la derniere pile d'un type adverse termine la partie : inutile
    // d'explorer quoi que ce soit d'autre. Tres rentable en finale.
    {
        Move wc = pos_.winning_capture(side);
        if (wc != MOVE_NONE) {
            if (params().useTT) tt_.store(key_of(side, phase), VALUE_MATE - ply - 1, wc, depth, TT_LOWER);
            pvLen_[ply] = 1; pv_[ply][0] = wc;
            return VALUE_MATE - ply - 1;
        }
    }

    // ---- table de transposition --------------------------------------------
    const u64 key = key_of(side, phase);
    Move ttMove = MOVE_NONE;
    if (params().useTT) {
        TTEntry e;
        if (tt_.probe(key, e)) {
            st_->ttHits++;
            ttMove = e.move;
            if (e.depth >= depth) {
                int s = e.score;
                if (s >  VALUE_MATE_MIN) s -= ply;      // scores de mat relatifs
                if (s < -VALUE_MATE_MIN) s += ply;
                if (e.flag == TT_EXACT) return s;
                if (e.flag == TT_LOWER && s >= beta)  return s;
                if (e.flag == TT_UPPER && s <= alpha) return s;
            }
        }
    }

    // ---- generation en deux listes (un seul balayage) -----------------------
    Move caps[MAX_MOVES], quiets[MAX_MOVES];
    int  nCaps = 0, nQuiets = 0;
    pos_.gen_split(caps, nCaps, quiets, nQuiets, side);

    // Defaite du referee n.3 : en phase 1, ne pas pouvoir capturer fait perdre.
    if (phase == PHASE_CAPTURE && nCaps == 0) return -VALUE_MATE + ply;

    // ---- feuille : uniquement sur une frontiere de tour ---------------------
    // Evaluer entre la phase 1 et la phase 2 fausserait le score (le camp au
    // trait a encore une action gratuite) : on ne s'arrete qu'en debut de tour.
    if (depth <= 0 && phase == PHASE_CAPTURE) {
        st_->leaves++;
        return eval_leaf(pos_, side, caps, nCaps, alpha, beta);
    }

    refresh_worth();
    const int totalMoves = nCaps + (phase == PHASE_FREE ? nQuiets + 1 : 0);
    // Coup unique force -> extension (frequent dans les finales de TZAAR).
    const int ext = (totalMoves == 1) ? 1 : 0;

    const int alphaOrig = alpha;
    int  best     = -VALUE_INF;
    Move bestMove = MOVE_NONE;
    const EvalParams& P = params();

    // Quota d'empilements reellement explores (Late Move Pruning). La phase 2
    // propose ~145 coups dont l'immense majorite sont des empilements inutiles.
    const int quietCap = (P.useLMP && depth > 0 && !is_mate_score(alpha))
                       ? P.lmpBase + P.lmpFactor * depth : 1 << 30;

    bool cutoff = false;

    // --- fonction locale : joue un coup et met a jour la fenetre -------------
    auto try_move = [&](Move m, bool quiet, int moveIndex) -> bool {
        Undo u;
        pos_.do_move(m, u);

        int red = 0;
        if (P.useLMR && quiet && depth >= P.lmrMinDepth && moveIndex >= P.lmrMinMove && !ext)
            red = 1 + (moveIndex >= P.lmrMinMove * 3 ? 1 : 0);

        const int newDepth = depth - 1 + ext;
        // Principal Variation Search : seul le premier coup merite une fenetre
        // complete ; les suivants sont d'abord refutes a fenetre nulle, ce qui
        // coupe beaucoup plus vite. Re-recherche complete uniquement si l'un
        // d'eux dement la prevision.
        const bool pvNode = (moveIndex == 0);
        int score;
        if (phase == PHASE_CAPTURE) {
            // meme joueur : sa phase 2 enchaine, donc PAS de negation
            if (pvNode) score = negamax(newDepth, alpha, beta, side, PHASE_FREE, ply + 1);
            else {
                score = negamax(newDepth - red, alpha, alpha + 1, side, PHASE_FREE, ply + 1);
                if (!stop_ && score > alpha && (red || score < beta))
                    score = negamax(newDepth, alpha, beta, side, PHASE_FREE, ply + 1);
            }
        } else {
            // fin de tour : la main change, negamax classique
            if (pvNode) score = -negamax(newDepth, -beta, -alpha, opp, PHASE_CAPTURE, ply + 1);
            else {
                score = -negamax(newDepth - red, -alpha - 1, -alpha, opp, PHASE_CAPTURE, ply + 1);
                if (!stop_ && score > alpha && (red || score < beta))
                    score = -negamax(newDepth, -beta, -alpha, opp, PHASE_CAPTURE, ply + 1);
            }
        }

        pos_.undo_move(u);
        if (stop_) return true;

        if (score > best) {
            best = score; bestMove = m;
            if (score > alpha) { alpha = score; update_pv(ply, m); }
        }
        if (alpha >= beta) {
            st_->betaCuts++;
            if (moveIndex == 0) st_->firstCuts++;
            if (quiet) {
                if (killers_[ply][0] != m) { killers_[ply][1] = killers_[ply][0]; killers_[ply][0] = m; }
                if (is_real(m)) history_[side][move_from(m)][move_to(m)] += depth * depth;
            }
            cutoff = true;
            return true;
        }
        return false;
    };

    int played = 0;

    // --- etage 1 : captures (toujours generees, toujours prioritaires) -------
    {
        int sc[MAX_MOVES];
        for (int i = 0; i < nCaps; ++i) {
            const int to = move_to(caps[i]);
            sc[i] = (caps[i] == ttMove) ? ORD_TT
                  : ORD_CAPTURE + worth_[opp][pos_.type_[to]] + 8 * pos_.height_[to];
        }
        // Quota de captures explorees (0 = illimite). Les captures etant
        // triees, celles qu'on coupe sont les moins prometteuses.
        const int capCap = (P.capBase > 0 && depth > 0 && !is_mate_score(alpha))
                         ? P.capBase + P.capFactor * depth : 1 << 30;
        const int limit = nCaps < capCap ? nCaps : capCap;
        for (int i = 0; i < limit && !cutoff; ++i) {
            int bi = i;
            for (int j = i + 1; j < nCaps; ++j) if (sc[j] > sc[bi]) bi = j;
            std::swap(caps[i], caps[bi]); std::swap(sc[i], sc[bi]);
            if (try_move(caps[i], false, played++)) break;
        }
        if (stop_) return 0;
    }

    // --- etage 2 : WAIT, puis les empilements -------------------------------
    if (!cutoff && phase == PHASE_FREE) {
        if (!try_move(MOVE_WAIT, true, played++) && !stop_) {
            int sc[MAX_MOVES];
            for (int i = 0; i < nQuiets; ++i) sc[i] = score_move(quiets[i], side, ttMove, ply);
            int quietSeen = 0;
            for (int i = 0; i < nQuiets && !cutoff; ++i) {
                int bi = i;
                for (int j = i + 1; j < nQuiets; ++j) if (sc[j] > sc[bi]) bi = j;
                std::swap(quiets[i], quiets[bi]); std::swap(sc[i], sc[bi]);
                // Les empilements sont tries : une fois le quota atteint, tout
                // ce qui reste vaut moins -> on sort net.
                if (++quietSeen > quietCap) break;
                if (try_move(quiets[i], true, played++)) break;
            }
        }
        if (stop_) return 0;
    }

    if (bestMove == MOVE_NONE) return -VALUE_MATE + ply;   // filet de securite

    if (params().useTT) {
        TTFlag f = (best <= alphaOrig) ? TT_UPPER : (best >= beta ? TT_LOWER : TT_EXACT);
        int s = best;
        if (s >  VALUE_MATE_MIN) s += ply;
        if (s < -VALUE_MATE_MIN) s -= ply;
        tt_.store(key, s, bestMove, depth, f);
    }
    return best;
}

// ---------------------------------------------------------------------------
Move Searcher::think(const Position& pos, int side, int phase,
                     bool firstActionOfGame, const SearchLimits& lim, SearchStats& st) {
    pos_ = pos;
    st_  = &st;
    st   = SearchStats{};
    st_->best = MOVE_NONE;
    stop_ = false;
    budgetUs_ = lim.timeUs;
    i64 soft = lim.softTimeUs > 0 ? lim.softTimeUs : (lim.timeUs * 3) / 5;
    timer_.start();
    eval_rebuild_tables();   // les parametres peuvent avoir change (tuning/arene)
    tt_.new_generation();
    std::memset(killers_, 0xFF, sizeof killers_);
    for (int i = 0; i < MAX_PLY; ++i) pvLen_[i] = 0;

    const int opp = side ^ 1;

    Move rootMoves[MAX_MOVES];
    int  n = pos_.gen_moves(rootMoves, side, phase);
    if (n == 0) return phase == PHASE_FREE ? MOVE_WAIT : MOVE_NONE;

    st.best = rootMoves[0];

    int prevScore = 0;
    for (int depth = 1; depth <= lim.maxDepth; ++depth) {
        if (depth > 1 && timer_.us() > soft) break;

        // fenetre d'aspiration autour du score precedent
        int window = params().aspirationWindow;
        int alpha = (depth <= 2 || is_mate_score(prevScore)) ? -VALUE_INF : prevScore - window;
        int beta  = (depth <= 2 || is_mate_score(prevScore)) ?  VALUE_INF : prevScore + window;

        int  iterScore = -VALUE_INF;
        Move iterBest  = MOVE_NONE;
        bool failed;

        do {
            failed = false;
            iterScore = -VALUE_INF;
            iterBest  = MOVE_NONE;

            // TT move en tete pour cette iteration
            Move ttMove = MOVE_NONE;
            { TTEntry e; if (tt_.probe(pos_.key(side, phase), e)) ttMove = e.move; }
            int sc[MAX_MOVES];
            for (int i = 0; i < n; ++i) sc[i] = score_move(rootMoves[i], side, ttMove, 0);
            for (int i = 0; i < n; ++i) {
                int bi = i;
                for (int j = i + 1; j < n; ++j) if (sc[j] > sc[bi]) bi = j;
                std::swap(rootMoves[i], rootMoves[bi]); std::swap(sc[i], sc[bi]);
            }

            int a = alpha;
            for (int i = 0; i < n; ++i) {
                Move m = rootMoves[i];
                Undo u; pos_.do_move(m, u);
                int score;
                if (phase == PHASE_CAPTURE && firstActionOfGame) {
                    // tout premier coup de la partie : pas de phase 2, la main passe
                    score = -negamax(depth - 1, -beta, -a, opp, PHASE_CAPTURE, 1);
                } else if (phase == PHASE_CAPTURE) {
                    score =  negamax(depth - 1, a, beta, side, PHASE_FREE, 1);
                } else {
                    score = -negamax(depth - 1, -beta, -a, opp, PHASE_CAPTURE, 1);
                }
                pos_.undo_move(u);
                if (stop_) break;

                if (score > iterScore) {
                    iterScore = score; iterBest = m;
                    if (score > a) { a = score; update_pv(0, m); }
                }
                if (a >= beta) break;
            }

            if (stop_) break;
            if (iterScore <= alpha && alpha != -VALUE_INF) {          // fail low
                alpha = -VALUE_INF; failed = true;
            } else if (iterScore >= beta && beta != VALUE_INF) {      // fail high
                beta = VALUE_INF; failed = true;
            }
        } while (failed);

        if (stop_) break;

        st.depth = depth;
        st.score = iterScore;
        if (is_real(iterBest) || is_wait(iterBest)) st.best = iterBest;
        prevScore = iterScore;

        st.pv.clear();
        for (int i = 0; i < pvLen_[0]; ++i) {
            if (i) st.pv += " ";
            st.pv += move_to_string(pv_[0][i]);
        }
        TRACE("depth=%d score=%d nodes=%llu time=%.1fms pv=%s",
              depth, iterScore, (unsigned long long)st.nodes, timer_.ms(), st.pv.c_str());

        if (is_mate_score(iterScore)) { st.proven = true; break; }  // issue prouvee
    }

    st.elapsedMs = timer_.ms();
    return st.best;
}

} // namespace tzaar

// ============================================================================
//  Protocole CodinGame. Isole ici pour que le moteur reste testable hors I/O.
//
//   init  : myId
//   tour  : actionPhase / validPieceCount / (x y owner type height) * n
//   sortie: "MOVE x1 y1 x2 y2" ou "WAIT"
// ============================================================================
#ifndef TZAAR_IO_PROTOCOL_HPP
#define TZAAR_IO_PROTOCOL_HPP


namespace tzaar {

struct TurnInput {
    int phase = PHASE_CAPTURE;
    Position pos;
    bool ok = false;
};

// Lit un tour complet. Renvoie ok=false sur EOF (fin de partie).
TurnInput read_turn(std::istream& in);

} // namespace tzaar
#endif


namespace tzaar {

TurnInput read_turn(std::istream& in) {
    TurnInput t;
    int n;
    if (!(in >> t.phase >> n)) return t;
    t.pos.clear();
    for (int i = 0; i < n; ++i) {
        int x, y, ow, ty, h;
        if (!(in >> x >> y >> ow >> ty >> h)) return t;
        int c = cell_from_xy(x, y);
        if (c < 0) continue;                 // ne devrait pas arriver
        t.pos.owner_[c]  = (u8)ow;
        t.pos.type_[c]   = (u8)ty;
        t.pos.height_[c] = (u8)h;
        t.pos.occ |= 1ull << c;
    }
    t.pos.refresh();
    t.ok = true;
    return t;
}

} // namespace tzaar


namespace tzaar {

void trace_printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
}

static const char* TYPE_CHAR = "otZ";   // tott / tzarra / TZAAR

std::string board_to_string(const Position& pos, int hlFrom, int hlTo) {
    std::ostringstream o;
    o << "     x,y = (q+4, r+4)   J0=minuscule-A  J1=MAJUSCULE-B\n";
    for (int rr = -BOARD_R; rr <= BOARD_R; ++rr) {
        int qmin = -BOARD_R > -BOARD_R - rr ? -BOARD_R : -BOARD_R - rr;
        int qmax =  BOARD_R <  BOARD_R - rr ?  BOARD_R :  BOARD_R - rr;
        o << std::setw(3) << (rr + BOARD_R) << " ";
        for (int k = 0; k < (BOARD_R - (qmax - qmin)); ++k) o << "  ";
        for (int qq = qmin; qq <= qmax; ++qq) {
            if (qq == 0 && rr == 0) { o << "  .  "; continue; }
            int c = GEO().index[qq + BOARD_R][rr + BOARD_R];
            if (pos.empty_at(c)) { o << (c == hlTo ? "  +  " : "  -  "); continue; }
            char mark = (c == hlFrom) ? '<' : (c == hlTo ? '>' : ' ');
            o << mark << (pos.owner_[c] == 0 ? 'A' : 'B') << TYPE_CHAR[pos.type_[c]]
              << (char)('0' + (pos.height_[c] > 9 ? 9 : pos.height_[c])) << ' ';
        }
        o << "\n";
    }
    o << "counts J0: TOTT=" << pos.counts[0][0] << " TZARRA=" << pos.counts[0][1]
      << " TZAAR=" << pos.counts[0][2] << " pieces=" << pos.pieces[0] << "\n";
    o << "counts J1: TOTT=" << pos.counts[1][0] << " TZARRA=" << pos.counts[1][1]
      << " TZAAR=" << pos.counts[1][2] << " pieces=" << pos.pieces[1] << "\n";
    return o.str();
}

std::string position_to_input(const Position& pos) {
    std::ostringstream o;
    o << popcnt(pos.occ) << "\n";
    u64 b = pos.occ;
    while (b) {
        int c = pop_lsb(b);
        o << cell_x(c) << " " << cell_y(c) << " " << (int)pos.owner_[c] << " "
          << (int)pos.type_[c] << " " << (int)pos.height_[c] << "\n";
    }
    return o.str();
}

bool position_from_input(Position& pos, const std::string& text) {
    std::istringstream in(text);
    int n; if (!(in >> n)) return false;
    pos.clear();
    for (int i = 0; i < n; ++i) {
        int x, y, ow, ty, h;
        if (!(in >> x >> y >> ow >> ty >> h)) return false;
        int c = cell_from_xy(x, y);
        if (c < 0) return false;
        pos.owner_[c] = (u8)ow; pos.type_[c] = (u8)ty; pos.height_[c] = (u8)h;
        pos.occ |= 1ull << c;
    }
    pos.refresh();
    return true;
}

bool check_invariants(const Position& pos, std::string* why) {
    Position ref = pos;
    ref.refresh();
    auto fail = [&](const char* m) { if (why) *why = m; return false; };
    if (ref.hash != pos.hash)       return fail("hash desynchronise");
    if (ref.occP[0] != pos.occP[0] || ref.occP[1] != pos.occP[1]) return fail("occP desynchronise");
    for (int p = 0; p < 2; ++p) {
        if (ref.stacks[p] != pos.stacks[p])       return fail("stacks");
        if (ref.pieces[p] != pos.pieces[p])       return fail("pieces");
        if (ref.tallBonus[p] != pos.tallBonus[p]) return fail("tallBonus");
        for (int t = 0; t < NUM_TYPES; ++t)
            if (ref.counts[p][t] != pos.counts[p][t]) return fail("counts");
    }
    if (why) why->clear();
    return true;
}

} // namespace tzaar

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

