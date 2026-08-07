// ============================================================================
//  TZAAR solver - core types
//  Aucun include du projet ici : c'est la racine de la hierarchie.
// ============================================================================
#ifndef TZAAR_CORE_TYPES_HPP
#define TZAAR_CORE_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace tzaar {

using u8  = std::uint8_t;   using i8  = std::int8_t;
using u16 = std::uint16_t;  using i16 = std::int16_t;
using u32 = std::uint32_t;  using i32 = std::int32_t;
using u64 = std::uint64_t;  using i64 = std::int64_t;

// ---------------------------------------------------------------- plateau ---
// Hexagone de rayon 4 (61 cases) moins le centre  ->  60 cases.
constexpr int NUM_CELLS  = 60;
constexpr int NUM_DIRS   = 6;
constexpr int NUM_TYPES  = 3;
constexpr int BOARD_R    = 4;
// Une pile ne peut pas depasser les 30 pieces d'un joueur.
constexpr int MAX_HEIGHT = 30;

enum PieceType : u8 { TOTT = 0, TZARRA = 1, TZAAR = 2 };

// Le referee appelle le bot deux fois par tour.
//   phase 1 : capture OBLIGATOIRE (sinon defaite)
//   phase 2 : capture, empilement, ou WAIT
enum Phase : u8 { PHASE_CAPTURE = 1, PHASE_FREE = 2 };

// ------------------------------------------------------------------ coups ---
// 12 bits utiles : from(6) | to(6).  Deux sentinelles hors de cet espace.
using Move = u16;
constexpr Move MOVE_NONE = 0xFFFF;
constexpr Move MOVE_WAIT = 0xFFFE;

constexpr Move mk_move(int from, int to) { return (Move)((from << 6) | to); }
constexpr int  move_from(Move m)         { return (m >> 6) & 63; }
constexpr int  move_to  (Move m)         { return m & 63; }
constexpr bool is_wait  (Move m)         { return m == MOVE_WAIT; }
constexpr bool is_real   (Move m)        { return m < 0xF000; }

// 30 piles x 6 directions + WAIT = 181 au maximum absolu.
constexpr int MAX_MOVES = 200;
constexpr int MAX_PLY   = 160;   // la partie est bornee : ~120 demi-coups

// ------------------------------------------------------------------ scores --
constexpr int VALUE_MATE     = 1000000;
constexpr int VALUE_INF      = 2000000;
constexpr int VALUE_MATE_MIN = VALUE_MATE - MAX_PLY;   // seuil "mat trouve"

constexpr bool is_mate_score(int v) { return v >= VALUE_MATE_MIN || v <= -VALUE_MATE_MIN; }

// --------------------------------------------------------------- bit utils --
inline int  lsb  (u64 b) { return __builtin_ctzll(b); }
inline int  msb  (u64 b) { return 63 - __builtin_clzll(b); }
inline int  popcnt(u64 b){ return __builtin_popcountll(b); }
inline int  pop_lsb(u64& b) { int i = lsb(b); b &= b - 1; return i; }

} // namespace tzaar
#endif
