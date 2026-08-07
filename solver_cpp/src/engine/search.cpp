#include "search.hpp"
#include "../debug/trace.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace tzaar {

// ---- bornes d'ordonnancement (decroissantes) -------------------------------
static constexpr int ORD_TT       = 1 << 30;
static constexpr int ORD_WINNING  = 1 << 28;   // capture la derniere piece d'un type
static constexpr int ORD_CAPTURE  = 1 << 20;
static constexpr int ORD_KILLER   = 1 << 19;
static constexpr int ORD_QUIET    = 1 << 10;

std::string move_to_string(Move m) {
    if (is_wait(m))  return "WAIT";
    if (!is_real(m)) return "(none)";
    char buf[40];
    std::snprintf(buf, sizeof buf, "MOVE %d %d %d %d",
                  cell_x(move_from(m)), cell_y(move_from(m)),
                  cell_x(move_to(m)),   cell_y(move_to(m)));
    return buf;
}

// Cle TT : la position seule ne suffit pas, il faut le camp au trait ET la
// phase (les coups legaux en dependent).
inline u64 Searcher::key_of(int side, int phase) const { return pos_.key(side, phase); }

Searcher::Searcher() {
    tt_.resize_mb(64);
    new_game();
}

void Searcher::set_hash_mb(size_t mb) { tt_.resize_mb(mb); }

void Searcher::new_game() {
    tt_.clear();
    std::memset(killers_, 0xFF, sizeof killers_);
    std::memset(history_, 0, sizeof history_);
}

// ---------------------------------------------------------------------------
void Searcher::refresh_worth() {
    for (int o = 0; o < 2; ++o)
        for (int t = 0; t < NUM_TYPES; ++t)
            worth_[o][t] = piece_worth(t, pos_.counts[o][t]);
}

// Les coups sont classes en 4 paliers strictement disjoints :
//   TT > captures > WAIT/killers > empilements.
// Les paliers etant ordonnes, des qu'on atteint un empilement tous les
// suivants en sont aussi : la boucle peut sortir net (voir LMP).
int Searcher::score_move(Move m, int side, Move ttMove, int ply) const {
    if (m == ttMove) return ORD_TT;
    // WAIT en tete des coups tranquilles : c'est l'option "ne rien risquer",
    // souvent correcte en phase 2, et elle ne doit jamais etre elaguee.
    if (is_wait(m))  return ORD_KILLER + 2;

    const int from = move_from(m), to = move_to(m);
    const int tt   = pos_.type_[to];

    if (pos_.owner_[to] != side)
        return ORD_CAPTURE + worth_[side ^ 1][tt] + 8 * pos_.height_[to];

    if (m == killers_[ply][0]) return ORD_KILLER + 1;
    if (m == killers_[ply][1]) return ORD_KILLER;

    // Empilement : gain de hauteur plafonne moins la valeur du type enterre.
    const int hf = pos_.height_[from], ht = pos_.height_[to];
    int v = params().heightWeight * (height_bonus(hf + ht) - height_bonus(hf) - height_bonus(ht))
          - worth_[side][tt];
    if (v < -4000) v = -4000; else if (v > 4000) v = 4000;
    int h = history_[side][from][to];
    if (h > (1 << 15)) h = 1 << 15;
    return ORD_QUIET + v * 8 + h;
}

void Searcher::update_pv(int ply, Move m) {
    pv_[ply][0] = m;
    int n = pvLen_[ply + 1];
    for (int i = 0; i < n; ++i) pv_[ply][i + 1] = pv_[ply + 1][i];
    pvLen_[ply] = n + 1;
}

// ---------------------------------------------------------------------------
int Searcher::negamax(int depth, int alpha, int beta, int side, int phase, int ply) {
    st_->nodes++;
    if (ply > st_->seldepth) st_->seldepth = ply;
    pvLen_[ply] = 0;

    // Controle du temps tous les 512 noeuds : a ~1,5 M noeuds/s cela borne le
    // depassement a ~0,3 ms, negligeable devant la marge de securite.
    if ((st_->nodes & 511) == 0 && timer_.us() >= budgetUs_) { stop_ = true; return 0; }
    if (ply >= MAX_PLY - 2) return eval(pos_, side, alpha, beta);

    const int opp = side ^ 1;

    // ---- conditions de fin, dans l'ordre exact du referee -------------------
    // 1) le joueur au trait a perdu tous ses exemplaires d'un type
    if (pos_.missing_type(side) >= 0) return -VALUE_MATE + ply;
    // 2) idem pour l'adversaire : irreversible (on ne deterre jamais une piece)
    if (pos_.missing_type(opp) >= 0)  return  VALUE_MATE - ply;

    // ---- gain immediat prouve ----------------------------------------------
    // Prendre la derniere pile d'un type adverse termine la partie : inutile
    // d'explorer quoi que ce soit d'autre. Tres rentable en finale.
    {
        Move wc = pos_.winning_capture(side);
        if (wc != MOVE_NONE) {
            if (params().useTT) tt_.store(key_of(side, phase), VALUE_MATE - ply - 1, wc, depth, TT_LOWER);
            pvLen_[ply] = 1; pv_[ply][0] = wc;
            return VALUE_MATE - ply - 1;
        }
    }

    // ---- table de transposition --------------------------------------------
    const u64 key = key_of(side, phase);
    Move ttMove = MOVE_NONE;
    if (params().useTT) {
        TTEntry e;
        if (tt_.probe(key, e)) {
            st_->ttHits++;
            ttMove = e.move;
            if (e.depth >= depth) {
                int s = e.score;
                if (s >  VALUE_MATE_MIN) s -= ply;      // scores de mat relatifs
                if (s < -VALUE_MATE_MIN) s += ply;
                if (e.flag == TT_EXACT) return s;
                if (e.flag == TT_LOWER && s >= beta)  return s;
                if (e.flag == TT_UPPER && s <= alpha) return s;
            }
        }
    }

    // ---- generation en deux listes (un seul balayage) -----------------------
    Move caps[MAX_MOVES], quiets[MAX_MOVES];
    int  nCaps = 0, nQuiets = 0;
    pos_.gen_split(caps, nCaps, quiets, nQuiets, side);

    // Defaite du referee n.3 : en phase 1, ne pas pouvoir capturer fait perdre.
    if (phase == PHASE_CAPTURE && nCaps == 0) return -VALUE_MATE + ply;

    // ---- feuille : uniquement sur une frontiere de tour ---------------------
    // Evaluer entre la phase 1 et la phase 2 fausserait le score (le camp au
    // trait a encore une action gratuite) : on ne s'arrete qu'en debut de tour.
    if (depth <= 0 && phase == PHASE_CAPTURE) {
        st_->leaves++;
        return eval_leaf(pos_, side, caps, nCaps, alpha, beta);
    }

    refresh_worth();
    const int totalMoves = nCaps + (phase == PHASE_FREE ? nQuiets + 1 : 0);
    // Coup unique force -> extension (frequent dans les finales de TZAAR).
    const int ext = (totalMoves == 1) ? 1 : 0;

    const int alphaOrig = alpha;
    int  best     = -VALUE_INF;
    Move bestMove = MOVE_NONE;
    const EvalParams& P = params();

    // Quota d'empilements reellement explores (Late Move Pruning). La phase 2
    // propose ~145 coups dont l'immense majorite sont des empilements inutiles.
    const int quietCap = (P.useLMP && depth > 0 && !is_mate_score(alpha))
                       ? P.lmpBase + P.lmpFactor * depth : 1 << 30;

    bool cutoff = false;

    // --- fonction locale : joue un coup et met a jour la fenetre -------------
    auto try_move = [&](Move m, bool quiet, int moveIndex) -> bool {
        Undo u;
        pos_.do_move(m, u);

        int red = 0;
        if (P.useLMR && quiet && depth >= P.lmrMinDepth && moveIndex >= P.lmrMinMove && !ext)
            red = 1 + (moveIndex >= P.lmrMinMove * 3 ? 1 : 0);

        const int newDepth = depth - 1 + ext;
        // Principal Variation Search : seul le premier coup merite une fenetre
        // complete ; les suivants sont d'abord refutes a fenetre nulle, ce qui
        // coupe beaucoup plus vite. Re-recherche complete uniquement si l'un
        // d'eux dement la prevision.
        const bool pvNode = (moveIndex == 0);
        int score;
        if (phase == PHASE_CAPTURE) {
            // meme joueur : sa phase 2 enchaine, donc PAS de negation
            if (pvNode) score = negamax(newDepth, alpha, beta, side, PHASE_FREE, ply + 1);
            else {
                score = negamax(newDepth - red, alpha, alpha + 1, side, PHASE_FREE, ply + 1);
                if (!stop_ && score > alpha && (red || score < beta))
                    score = negamax(newDepth, alpha, beta, side, PHASE_FREE, ply + 1);
            }
        } else {
            // fin de tour : la main change, negamax classique
            if (pvNode) score = -negamax(newDepth, -beta, -alpha, opp, PHASE_CAPTURE, ply + 1);
            else {
                score = -negamax(newDepth - red, -alpha - 1, -alpha, opp, PHASE_CAPTURE, ply + 1);
                if (!stop_ && score > alpha && (red || score < beta))
                    score = -negamax(newDepth, -beta, -alpha, opp, PHASE_CAPTURE, ply + 1);
            }
        }

        pos_.undo_move(u);
        if (stop_) return true;

        if (score > best) {
            best = score; bestMove = m;
            if (score > alpha) { alpha = score; update_pv(ply, m); }
        }
        if (alpha >= beta) {
            st_->betaCuts++;
            if (moveIndex == 0) st_->firstCuts++;
            if (quiet) {
                if (killers_[ply][0] != m) { killers_[ply][1] = killers_[ply][0]; killers_[ply][0] = m; }
                if (is_real(m)) history_[side][move_from(m)][move_to(m)] += depth * depth;
            }
            cutoff = true;
            return true;
        }
        return false;
    };

    int played = 0;

    // --- etage 1 : captures (toujours generees, toujours prioritaires) -------
    {
        int sc[MAX_MOVES];
        for (int i = 0; i < nCaps; ++i) {
            const int to = move_to(caps[i]);
            sc[i] = (caps[i] == ttMove) ? ORD_TT
                  : ORD_CAPTURE + worth_[opp][pos_.type_[to]] + 8 * pos_.height_[to];
        }
        // Quota de captures explorees (0 = illimite). Les captures etant
        // triees, celles qu'on coupe sont les moins prometteuses.
        const int capCap = (P.capBase > 0 && depth > 0 && !is_mate_score(alpha))
                         ? P.capBase + P.capFactor * depth : 1 << 30;
        const int limit = nCaps < capCap ? nCaps : capCap;
        for (int i = 0; i < limit && !cutoff; ++i) {
            int bi = i;
            for (int j = i + 1; j < nCaps; ++j) if (sc[j] > sc[bi]) bi = j;
            std::swap(caps[i], caps[bi]); std::swap(sc[i], sc[bi]);
            if (try_move(caps[i], false, played++)) break;
        }
        if (stop_) return 0;
    }

    // --- etage 2 : WAIT, puis les empilements -------------------------------
    if (!cutoff && phase == PHASE_FREE) {
        if (!try_move(MOVE_WAIT, true, played++) && !stop_) {
            int sc[MAX_MOVES];
            for (int i = 0; i < nQuiets; ++i) sc[i] = score_move(quiets[i], side, ttMove, ply);
            int quietSeen = 0;
            for (int i = 0; i < nQuiets && !cutoff; ++i) {
                int bi = i;
                for (int j = i + 1; j < nQuiets; ++j) if (sc[j] > sc[bi]) bi = j;
                std::swap(quiets[i], quiets[bi]); std::swap(sc[i], sc[bi]);
                // Les empilements sont tries : une fois le quota atteint, tout
                // ce qui reste vaut moins -> on sort net.
                if (++quietSeen > quietCap) break;
                if (try_move(quiets[i], true, played++)) break;
            }
        }
        if (stop_) return 0;
    }

    if (bestMove == MOVE_NONE) return -VALUE_MATE + ply;   // filet de securite

    if (params().useTT) {
        TTFlag f = (best <= alphaOrig) ? TT_UPPER : (best >= beta ? TT_LOWER : TT_EXACT);
        int s = best;
        if (s >  VALUE_MATE_MIN) s += ply;
        if (s < -VALUE_MATE_MIN) s -= ply;
        tt_.store(key, s, bestMove, depth, f);
    }
    return best;
}

// ---------------------------------------------------------------------------
Move Searcher::think(const Position& pos, int side, int phase,
                     bool firstActionOfGame, const SearchLimits& lim, SearchStats& st) {
    pos_ = pos;
    st_  = &st;
    st   = SearchStats{};
    st_->best = MOVE_NONE;
    stop_ = false;
    budgetUs_ = lim.timeUs;
    i64 soft = lim.softTimeUs > 0 ? lim.softTimeUs : (lim.timeUs * 3) / 5;
    timer_.start();
    eval_rebuild_tables();   // les parametres peuvent avoir change (tuning/arene)
    tt_.new_generation();
    std::memset(killers_, 0xFF, sizeof killers_);
    for (int i = 0; i < MAX_PLY; ++i) pvLen_[i] = 0;

    const int opp = side ^ 1;

    Move rootMoves[MAX_MOVES];
    int  n = pos_.gen_moves(rootMoves, side, phase);
    if (n == 0) return phase == PHASE_FREE ? MOVE_WAIT : MOVE_NONE;

    st.best = rootMoves[0];

    int prevScore = 0;
    for (int depth = 1; depth <= lim.maxDepth; ++depth) {
        if (depth > 1 && timer_.us() > soft) break;

        // fenetre d'aspiration autour du score precedent
        int window = params().aspirationWindow;
        int alpha = (depth <= 2 || is_mate_score(prevScore)) ? -VALUE_INF : prevScore - window;
        int beta  = (depth <= 2 || is_mate_score(prevScore)) ?  VALUE_INF : prevScore + window;

        int  iterScore = -VALUE_INF;
        Move iterBest  = MOVE_NONE;
        bool failed;

        do {
            failed = false;
            iterScore = -VALUE_INF;
            iterBest  = MOVE_NONE;

            // TT move en tete pour cette iteration
            Move ttMove = MOVE_NONE;
            { TTEntry e; if (tt_.probe(pos_.key(side, phase), e)) ttMove = e.move; }
            int sc[MAX_MOVES];
            for (int i = 0; i < n; ++i) sc[i] = score_move(rootMoves[i], side, ttMove, 0);
            for (int i = 0; i < n; ++i) {
                int bi = i;
                for (int j = i + 1; j < n; ++j) if (sc[j] > sc[bi]) bi = j;
                std::swap(rootMoves[i], rootMoves[bi]); std::swap(sc[i], sc[bi]);
            }

            int a = alpha;
            for (int i = 0; i < n; ++i) {
                Move m = rootMoves[i];
                Undo u; pos_.do_move(m, u);
                int score;
                if (phase == PHASE_CAPTURE && firstActionOfGame) {
                    // tout premier coup de la partie : pas de phase 2, la main passe
                    score = -negamax(depth - 1, -beta, -a, opp, PHASE_CAPTURE, 1);
                } else if (phase == PHASE_CAPTURE) {
                    score =  negamax(depth - 1, a, beta, side, PHASE_FREE, 1);
                } else {
                    score = -negamax(depth - 1, -beta, -a, opp, PHASE_CAPTURE, 1);
                }
                pos_.undo_move(u);
                if (stop_) break;

                if (score > iterScore) {
                    iterScore = score; iterBest = m;
                    if (score > a) { a = score; update_pv(0, m); }
                }
                if (a >= beta) break;
            }

            if (stop_) break;
            if (iterScore <= alpha && alpha != -VALUE_INF) {          // fail low
                alpha = -VALUE_INF; failed = true;
            } else if (iterScore >= beta && beta != VALUE_INF) {      // fail high
                beta = VALUE_INF; failed = true;
            }
        } while (failed);

        if (stop_) break;

        st.depth = depth;
        st.score = iterScore;
        if (is_real(iterBest) || is_wait(iterBest)) st.best = iterBest;
        prevScore = iterScore;

        st.pv.clear();
        for (int i = 0; i < pvLen_[0]; ++i) {
            if (i) st.pv += " ";
            st.pv += move_to_string(pv_[0][i]);
        }
        TRACE("depth=%d score=%d nodes=%llu time=%.1fms pv=%s",
              depth, iterScore, (unsigned long long)st.nodes, timer_.ms(), st.pv.c_str());

        if (is_mate_score(iterScore)) { st.proven = true; break; }  // issue prouvee
    }

    st.elapsedMs = timer_.ms();
    return st.best;
}

} // namespace tzaar
