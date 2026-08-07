#include "protocol.hpp"

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
