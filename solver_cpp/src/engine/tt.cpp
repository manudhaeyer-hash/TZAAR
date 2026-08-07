#include "tt.hpp"

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
