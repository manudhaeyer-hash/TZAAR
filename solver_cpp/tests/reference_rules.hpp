// ============================================================================
//  Implementation NAIVE des regles, traduction litterale de Board.java.
//  Elle ne sert qu'aux tests : c'est l'oracle contre lequel on valide la
//  version bitboard. Volontairement lente et bete.
// ============================================================================
#ifndef TZAAR_TESTS_REFERENCE_RULES_HPP
#define TZAAR_TESTS_REFERENCE_RULES_HPP

#include "../src/core/position.hpp"
#include <vector>
#include <cmath>

namespace tzaar { namespace ref {

struct RCell { int q, r, s; };

inline bool on_board(int q, int r) {
    int s = -q - r;
    return std::abs(q) <= 4 && std::abs(r) <= 4 && std::abs(s) <= 4 && !(q == 0 && r == 0);
}

inline bool aligned(int aq, int ar, int bq, int br) {
    return aq == bq || ar == br || (-aq - ar) == (-bq - br);
}

inline int distance(int aq, int ar, int bq, int br) {
    int as = -aq - ar, bs = -bq - br;
    return (std::abs(aq - bq) + std::abs(ar - br) + std::abs(as - bs)) / 2;
}

// Traduction de Board.isPathClear : le trou central bloque la ligne.
inline bool path_clear(const Position& p, int aq, int ar, int bq, int br) {
    if (!aligned(aq, ar, bq, br)) return false;
    int d = distance(aq, ar, bq, br);
    if (d == 0) return true;
    int dq = (bq - aq) / d, dr = (br - ar) / d;
    int cq = aq + dq, cr = ar + dr;
    while (!(cq == bq && cr == br)) {
        if (cq == 0 && cr == 0) return false;
        int c = GEO().index[cq + 4][cr + 4];
        if (c >= 0 && !p.empty_at(c)) return false;
        cq += dq; cr += dr;
    }
    return true;
}

// Traduction de Board.isValidMove + la contrainte de phase du Referee.
inline bool valid_move(const Position& p, int from, int to, int side, int phase) {
    if (from == to) return false;
    if (p.empty_at(from) || p.empty_at(to)) return false;
    if (p.owner_[from] != side) return false;
    if (!path_clear(p, GEO().q[from], GEO().r[from], GEO().q[to], GEO().r[to])) return false;
    bool capture = p.owner_[to] != side;
    if (capture && p.height_[from] < p.height_[to]) return false;
    if (phase == PHASE_CAPTURE && !capture) return false;
    return true;
}

inline std::vector<Move> gen_moves(const Position& p, int side, int phase) {
    std::vector<Move> out;
    for (int f = 0; f < NUM_CELLS; ++f) {
        if (p.empty_at(f) || p.owner_[f] != side) continue;
        for (int t = 0; t < NUM_CELLS; ++t)
            if (valid_move(p, f, t, side, phase)) out.push_back(mk_move(f, t));
    }
    if (phase == PHASE_FREE) out.push_back(MOVE_WAIT);
    return out;
}

}} // namespace tzaar::ref
#endif
