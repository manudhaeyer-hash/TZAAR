#include "trace.hpp"
#include <cstdarg>
#include <sstream>
#include <iomanip>

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
