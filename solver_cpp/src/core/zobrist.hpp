// ============================================================================
//  Cles Zobrist. Une pile est identifiee par (case, proprietaire, type, hauteur)
//  car la hauteur fait partie de l'etat (elle change lors des empilements).
// ============================================================================
#ifndef TZAAR_CORE_ZOBRIST_HPP
#define TZAAR_CORE_ZOBRIST_HPP

#include "types.hpp"

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
