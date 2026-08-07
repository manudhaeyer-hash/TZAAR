// ============================================================================
//  Recherche alpha-beta.
//
//  STRUCTURE DE L'ARBRE - specifique a TZAAR :
//  un "coup" du referee = une demi-action. Le tour d'un joueur vaut deux
//  noeuds consecutifs du MEME camp (phase 1 puis phase 2), puis on passe a
//  l'adversaire. La negation negamax n'a donc lieu qu'a la transition
//  phase2 -> phase1. La fenetre alpha/beta est transmise telle quelle entre
//  phase 1 et phase 2 (meme joueur = deux noeuds maximisants d'affilee, ce que
//  l'alpha-beta gere sans probleme).
//
//  L'evaluation n'a lieu QUE sur une frontiere de tour (phase 1), ce qui
//  supprime tout biais pair/impair et rend une quiescence separee inutile.
//
//  PROPRIETE UTILE : la phase 1 impose une capture, donc chaque tour retire au
//  moins une pile du plateau. La partie est finie (~60 tours max), sans
//  repetition ni nulle par cycle : pas de detection de repetition necessaire,
//  et la recherche atteint naturellement des noeuds terminaux exacts en finale.
// ============================================================================
#ifndef TZAAR_ENGINE_SEARCH_HPP
#define TZAAR_ENGINE_SEARCH_HPP

#include "../core/position.hpp"
#include "eval.hpp"
#include "tt.hpp"
#include "timer.hpp"
#include <string>

namespace tzaar {

struct SearchLimits {
    int  maxDepth   = 64;      // en demi-actions
    i64  timeUs     = 40000;   // budget dur
    i64  softTimeUs = 0;       // 0 = 60% de timeUs : on n'entame pas une iteration
};

struct SearchStats {
    u64  nodes      = 0;
    u64  leaves     = 0;
    u64  ttHits     = 0;
    u64  betaCuts   = 0;
    u64  firstCuts  = 0;       // coupures sur le 1er coup -> qualite de l'ordre
    int  depth      = 0;
    int  seldepth   = 0;
    int  score      = 0;
    Move best       = MOVE_NONE;
    double elapsedMs = 0;
    std::string pv;
    bool  proven    = false;   // score de mat : issue prouvee
};

class Searcher {
public:
    Searcher();
    void set_hash_mb(size_t mb);
    void new_game();

    // firstActionOfGame : au tout premier coup, le joueur 0 ne joue QUE la
    // phase 1 puis la main passe (cf. Referee.gameTurn).
    Move think(const Position& pos, int side, int phase,
               bool firstActionOfGame, const SearchLimits& lim, SearchStats& st);

    TT& tt() { return tt_; }

private:
    int negamax(int depth, int alpha, int beta, int side, int phase, int ply);
    int  score_move(Move m, int side, Move ttMove, int ply) const;
    u64  key_of(int side, int phase) const;
    // Valeur marginale d'une pile de chaque (camp, type) dans la position
    // courante. Recalculee une fois par noeud au lieu d'une fois par coup :
    // evite ~145 divisions par noeud.
    void refresh_worth();
    int  worth_[2][NUM_TYPES];
    void update_pv(int ply, Move m);

    Position pos_;
    TT       tt_;
    Timer    timer_;
    i64      budgetUs_ = 0;
    bool     stop_     = false;
    SearchStats* st_   = nullptr;

    Move killers_[MAX_PLY][2];
    int  history_[2][NUM_CELLS][NUM_CELLS];

    Move pv_[MAX_PLY][MAX_PLY];
    int  pvLen_[MAX_PLY];
};

std::string move_to_string(Move m);

} // namespace tzaar
#endif
