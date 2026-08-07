// ============================================================================
//  Table de transposition. Entree de 16 octets, remplacement "profondeur ou
//  vieillesse". Taille en puissance de 2 -> indexation par masque.
// ============================================================================
#ifndef TZAAR_ENGINE_TT_HPP
#define TZAAR_ENGINE_TT_HPP

#include "../core/types.hpp"
#include <vector>

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
