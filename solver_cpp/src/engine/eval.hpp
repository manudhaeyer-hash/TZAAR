// ============================================================================
//  Evaluation. Deux etages :
//   1. eval_static  : O(1), n'utilise que les compteurs incrementaux.
//   2. eval_dynamic : un seul balayage du plateau (<= 60 x 6 sondes) qui produit
//      d'un coup la mobilite ET les menaces des DEUX camps.
//  eval() enchaine les deux avec un court-circuit "lazy" pilote par alpha/beta.
// ============================================================================
#ifndef TZAAR_ENGINE_EVAL_HPP
#define TZAAR_ENGINE_EVAL_HPP

#include "../core/position.hpp"
#include "eval_params.hpp"

namespace tzaar {

// Score du point de vue de `side`. Toujours appele sur une position non finie.
int eval_static (const Position& pos, int side);
int eval_full   (const Position& pos, int side);
int eval        (const Position& pos, int side, int alpha, int beta);

// Version feuille : la recherche vient de generer les captures du camp au
// trait, donc sa mobilite et ses menaces sont deja connues. Seul le camp
// adverse reste a balayer -> moitie moins de sondes que eval_full.
int eval_leaf(const Position& pos, int side, const Move* myCaps, int nMyCaps,
              int alpha, int beta);

// Reconstruit les tables derivees des parametres. A appeler apres toute
// modification de params().
void eval_rebuild_tables();

// --- primitives d'ordonnancement (toutes en O(1), sans division) ------------
int group_value (int type, int n);   // valeur totale d'un groupe de n piles
int piece_worth (int type, int n);   // valeur de la n-ieme pile : explose quand n est petit
int capture_value(const Position& pos, int victimCell);
int stack_value  (const Position& pos, int from, int to);

} // namespace tzaar
#endif
