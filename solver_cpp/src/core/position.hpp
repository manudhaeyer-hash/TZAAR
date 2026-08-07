// ============================================================================
//  Etat de jeu. Tout ce qui sert a l'evaluation est maintenu INCREMENTALEMENT
//  (compteurs par type, pieces totales, bonus de hauteur) : eval() est O(1)
//  hors terme de mobilite.
//
//  Regles encodees ici, verifiees contre Board.java / Referee.java :
//   - on ne se deplace QUE sur une case occupee (jamais sur du vide) ;
//   - la ligne doit etre libre, et le trou central bloque ;
//   - capture : hauteur source >= hauteur cible ; la source remplace la cible,
//     SANS gagner de hauteur ;
//   - empilement (pile alliee) : hauteurs additionnees, le type devient celui
//     de la pile qui arrive -> enterrer sa derniere piece d'un type FAIT PERDRE.
// ============================================================================
#ifndef TZAAR_CORE_POSITION_HPP
#define TZAAR_CORE_POSITION_HPP

#include "types.hpp"
#include "geometry.hpp"
#include "zobrist.hpp"

namespace tzaar {

// Bonus de hauteur plafonne : au-dela, empiler n'apporte plus rien.
constexpr int HEIGHT_BONUS_CAP = 5;
inline int height_bonus(int h) { int b = h - 1; return b > HEIGHT_BONUS_CAP ? HEIGHT_BONUS_CAP : b; }

struct Undo {
    Move move;
    u8   fromHeight;   // hauteur de la pile qui bouge (avant coup)
    u8   fromType;
    u8   toOwner, toType, toHeight;
    u64  hash;
};

struct Position {
    u64 occ;
    u64 occP[2];
    u8  owner_ [NUM_CELLS];
    u8  type_  [NUM_CELLS];
    u8  height_[NUM_CELLS];

    int counts   [2][NUM_TYPES]; // nb de PILES dont le sommet est de ce type
    int stacks   [2];            // nb de piles
    int pieces   [2];            // somme des hauteurs (pieces physiques)
    int tallBonus[2];            // somme des height_bonus()
    u64 hash;

    // ---------------------------------------------------------- construction
    void clear();
    void put(int cell, int owner, int type, int height);   // pose sans hash
    void refresh();                                        // recalcule hash + derives
    void init_start();                                     // position initiale officielle

    // -------------------------------------------------------------- lecture
    bool empty_at(int c) const { return !((occ >> c) & 1); }
    int  missing_type(int side) const {
        for (int t = 0; t < NUM_TYPES; ++t) if (counts[side][t] == 0) return t;
        return -1;
    }
    u64  key(int side, int phase) const { return hash ^ ZOB().side[side] ^ ZOB().phase[phase]; }

    // ------------------------------------------------------- generation ----
    int  gen_moves   (Move* out, int side, int phase) const;  // legaux complets
    int  gen_captures(Move* out, int side) const;
    // Un seul balayage produisant les deux listes : les captures d'un cote, les
    // empilements de l'autre. La recherche ne note et ne trie les empilements
    // que si aucune capture n'a provoque de coupure -> gros gain.
    void gen_split(Move* caps, int& nCaps, Move* quiets, int& nQuiets, int side) const;
    bool has_capture (int side) const;
    // Capture qui retire a l'adversaire sa DERNIERE pile d'un type : gain
    // immediat et prouve. Sortie rapide (aucun scan si aucun type critique).
    Move winning_capture(int side) const;
    bool is_legal    (Move m, int side, int phase) const;     // validation stricte

    // ------------------------------------------------------------ mutation --
    void do_move  (Move m, Undo& u);
    void undo_move(const Undo& u);

private:
    void add_stack(int cell, int owner, int type, int height);
    void del_stack(int cell);
};

} // namespace tzaar
#endif
