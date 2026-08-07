// ============================================================================
//  TOUS les poids de l'evaluation sont ici, dans une seule struct modifiable a
//  chaud. C'est le point d'entree du tuning : l'arene (tools/arena) peut faire
//  s'affronter deux jeux de parametres sans recompiler la recherche.
// ============================================================================
#ifndef TZAAR_ENGINE_EVAL_PARAMS_HPP
#define TZAAR_ENGINE_EVAL_PARAMS_HPP

#include "../core/types.hpp"

namespace tzaar {

struct EvalParams {
    // ---- valeur relative des trois types (TOTT / TZARRA / TZAAR) -----------
    // Il y a 15 TOTT, 9 TZARRA et 6 TZAAR par joueur : les TZAAR sont les plus
    // critiques parce qu'ils s'epuisent le plus vite.
    int typeValue[NUM_TYPES] = { 100, 118, 140 };

    // ---- courbe de rarete --------------------------------------------------
    // Valeur cumulee d'un groupe de N piles d'un meme type. Concave : passer de
    // 1 a 2 vaut enormement, passer de 10 a 11 presque rien. C'est ce qui
    // encode "perdre tous ses X = defaite" de facon continue.
    // Index 0 est un cas perdu gere en amont (score de mat).
    int countCurve[16] = { 0, 100, 196, 276, 340, 392, 434, 468, 494,
                           514, 530, 542, 552, 560, 566, 570 };
    int countCurveTail = 4;     // increment lineaire au-dela de l'index 15

    // ---- hauteur -----------------------------------------------------------
    // Une pile haute capture tout et ne peut etre prise que par plus haut
    // qu'elle, mais elle immobilise des pieces : gain plafonne.
    int heightWeight = 26;      // applique a tallBonus (somme des min(h-1,CAP))
    int pieceWeight  = 3;       // applique aux pieces physiques totales

    // ---- course aux pieces rares (DESACTIVE par defaut) --------------------
    // Constat : une evaluation a somme nulle juge un echange 1 contre 1 neutre,
    // et les deux moteurs echangent donc leurs TZAAR de 6-6 a 1-1 en dix coups.
    // Ce terme rend l'asymetrie du trait visible. Il n'est PAS a somme nulle,
    // mais reste correct en negamax : l'evaluation n'est appelee qu'en frontiere
    // de tour, toujours du point de vue du camp au trait.
    //
    // MESURE : 40 parties a 20 ms, raceTempo=260 raceEdge=180 -> 47,5 %.
    // Aucun gain mesurable, donc neutralise par defaut. Le levier reste en
    // place : c'est la premiere piste a reprendre avec un budget de parties
    // plus large ou d'autres poids.
    int raceThreshold = 3;    // "type presque epuise" : effectif <= ce seuil
    int raceTempo     = 0;    // a effectif egal, avoir le trait
    int raceEdge      = 0;    // par exemplaire d'avance sur un type rare

    // ---- termes dynamiques (scan du plateau, actives a la demande) ---------
    int mobilityWeight = 7;     // par capture disponible
    int threatWeight   = 14;    // par pile menacee, pondere par la rarete du type
    int zugzwangWeight = 260;   // bonus si l'adversaire n'a presque plus de captures

    // Marge de l'evaluation paresseuse : si le terme O(1) sort deja de la
    // fenetre alpha/beta de plus que ca, on n'evalue pas le terme dynamique.
    int lazyMargin = 420;
    bool useDynamic = true;

    // ---- recherche ---------------------------------------------------------
    int  aspirationWindow = 60;
    int  lmrMinDepth      = 3;
    int  lmrMinMove       = 4;
    bool useLMR           = true;
    bool useTT            = true;

    // Late Move Pruning : la phase 2 offre ~145 coups dont l'immense majorite
    // sont des empilements sans interet. On plafonne le nombre d'empilements
    // reellement explores en fonction de la profondeur restante.
    int  lmpBase   = 4;
    int  lmpFactor = 3;      // quiets explores = lmpBase + lmpFactor * depth
    bool useLMP    = true;

    // Meme idee sur les captures : la phase 1 en propose ~43, dont beaucoup
    // sont sans objet (prendre un pion isole avec sa plus grosse pile).
    // 0 = desactive.
    int  capBase   = 0;
    int  capFactor = 0;      // captures explorees = capBase + capFactor * depth
};

// Jeu de parametres courant du processus (modifiable pour le tuning).
extern EvalParams g_params;
inline EvalParams& params() { return g_params; }

} // namespace tzaar
#endif
