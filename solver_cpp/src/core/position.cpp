#include "position.hpp"
#include <cstring>

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
