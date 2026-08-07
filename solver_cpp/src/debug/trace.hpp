// ============================================================================
//  Outils de debug. Tout passe par stderr : sur CodinGame stderr est affiche
//  dans la console sans polluer le protocole (stdout).
//
//  Compiler avec -DTZAAR_TRACE=1 pour activer TRACE().
//  Compiler avec -DTZAAR_ASSERT=1 pour activer les invariants (couteux).
// ============================================================================
#ifndef TZAAR_DEBUG_TRACE_HPP
#define TZAAR_DEBUG_TRACE_HPP

#include "../core/position.hpp"
#include <string>
#include <cstdio>

namespace tzaar {

// Rendu ASCII du plateau hexagonal (lignes r = -4..4, indentees).
std::string board_to_string(const Position& pos, int highlightFrom = -1, int highlightTo = -1);
// Dump compact rejouable : une ligne par pile, format identique a l'entree CG.
std::string position_to_input(const Position& pos);
// Relit un dump produit par position_to_input().
bool position_from_input(Position& pos, const std::string& text);

void trace_printf(const char* fmt, ...);

// Verifie que les compteurs incrementaux collent au plateau reel.
bool check_invariants(const Position& pos, std::string* why = nullptr);

} // namespace tzaar

#if defined(TZAAR_TRACE) && TZAAR_TRACE
  #define TRACE(...) ::tzaar::trace_printf(__VA_ARGS__)
#else
  #define TRACE(...) ((void)0)
#endif

#if defined(TZAAR_ASSERT) && TZAAR_ASSERT
  #include <cstdlib>
  #define TZ_ASSERT(cond, msg) do { if (!(cond)) { \
        std::fprintf(stderr, "ASSERT %s:%d: %s (%s)\n", __FILE__, __LINE__, msg, #cond); \
        std::abort(); } } while (0)
#else
  #define TZ_ASSERT(cond, msg) ((void)0)
#endif

#endif
