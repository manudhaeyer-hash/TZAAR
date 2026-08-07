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

#include "types.hpp"

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
