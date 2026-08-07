#include "zobrist.hpp"

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
