#include "geometry.hpp"

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
