# TZAAR — solver C++

Bot alpha-bêta pour le jeu TZAAR de ce dépôt. Aucun fichier existant du projet
n'est modifié : tout est contenu dans `solver_cpp/`.

Trois objectifs, dans cet ordre : **correct**, **débuggable**, **rapide**.

---

## Démarrage

```powershell
# build (Makefile, sans dépendance)
make                      # bin/tzaar, tzaar_arena, tzaar_bench, tzaar_perft, tzaar_tests
make test                 # conformité des règles contre un oracle naïf
make bench                # nœuds/s et profondeur atteinte
make arena                # 20 parties de self-play

# ou avec CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# fichier unique à coller dans l'IDE CodinGame
python3 tools/bundle.py       # -> submit/tzaar_bot.cpp
.\tools\bundle.ps1            # équivalent PowerShell
```

`submit/tzaar_bot.cpp` est un `.cpp` autonome (~1600 lignes) qui compile seul.
**Ne jamais l'éditer** : il est régénéré depuis `src/`.

---

## Ce que le référé dit vraiment

Ces points viennent de la lecture de `Board.java` / `Referee.java`, pas de
l'énoncé. Ils conditionnent toute la conception :

| Point | Conséquence |
|---|---|
| Le trou central **bloque** les lignes (`isPathClear` refuse `(0,0)`) | Les rayons précalculés s'arrêtent avant le centre |
| Empiler écrase le type : le sommet devient celui de la pile qui **arrive** | **Enterrer sa dernière pièce d'un type fait perdre** — c'est un coup légal et suicidaire |
| Capturer ne fait **pas** grandir la pile ; seul l'empilement additionne | Hauteur et matériel sont deux ressources distinctes |
| La phase 1 impose une capture, donc **chaque tour retire une pile du plateau** | La partie est bornée (~60 tours), sans cycle ni répétition possible. Pas de détection de répétition, et les finales se résolvent **exactement** |
| `getMissingPieceType` regarde le **sommet** des piles | Les compteurs suivent les sommets, pas les pièces physiques |
| Au tout premier coup, le joueur 0 ne joue **que** la phase 1 | Cas particulier câblé à la racine (`firstActionOfGame`) |

Ces règles sont verrouillées par `tests/test_rules.cpp`, qui compare la
génération de coups à une traduction littérale de `Board.java`
(`tests/reference_rules.hpp`) sur ~10 000 positions issues de parties aléatoires.

---

## Architecture

```
src/core/     géométrie, plateau, génération de coups, Zobrist   (aucune dépendance)
src/engine/   évaluation, paramètres, table de transposition, recherche
src/io/       protocole CodinGame (isolé : le moteur est testable sans I/O)
src/debug/    rendu ASCII, dumps rejouables, vérification d'invariants
src/main_cg.cpp   le bot
tools/        arena (self-play), bench, perft, referee.py, bundlers
tests/        conformité des règles + oracle naïf
```

### Représentation

Les 60 cases tiennent dans un `uint64_t`. Elles sont numérotées **par `r`
croissant puis `q` croissant** ; avec cet ordre, trois des six directions font
toujours croître l'indice et les trois autres toujours décroître. Trouver la
première pile sur une ligne devient :

```cpp
u64 b = occ & ray[cell][dir];
int cible = (dir < 3) ? lsb(b) : msb(b);      // 2 instructions, sans boucle
```

Le test `test_geometry` vérifie cette monotonie — elle est la fondation de tout
le reste.

Tout ce que l'évaluation consomme (compteurs par type, pièces, bonus de hauteur,
hash) est maintenu **incrémentalement** dans `do_move`/`undo_move`.

### Recherche

Un « coup » du référé est une demi-action. Le tour d'un joueur vaut deux nœuds
consécutifs du **même** camp (phase 1 puis phase 2) : la négation negamax n'a
donc lieu qu'à la transition phase 2 → phase 1.

L'évaluation n'est appelée **que sur une frontière de tour**. Évaluer entre la
phase 1 et la phase 2 fausserait le score (le camp au trait a encore une action
gratuite) ; cette contrainte supprime tout biais pair/impair et rend une
quiescence séparée inutile.

Techniques en place :

- approfondissement itératif + fenêtres d'aspiration
- table de transposition Zobrist (clé = position + camp + phase)
- PVS (fenêtre nulle sur tous les coups sauf le premier)
- génération **par étages** : captures d'abord ; les ~100 empilements ne sont
  ni notés ni triés si une capture provoque une coupure
- ordonnancement : coup de la TT → captures par rareté du type pris → WAIT /
  killers → empilements (gain de hauteur moins le type enterré) + historique
- LMR sur les empilements tardifs, LMP (quota d'empilements par profondeur)
- extension sur coup unique forcé
- **détection de gain immédiat** : si une capture prend la dernière pile d'un
  type adverse, le nœud renvoie un score de mat sans rien explorer

Le taux de coupure sur le premier coup mesuré est de ~99 %, c'est-à-dire un
arbre quasi minimal : la profondeur est limitée par le facteur de branchement
(≈ 43 captures × ≈ 145 coups de phase 2 par tour), pas par l'ordonnancement.

### Évaluation

Deux étages, avec court-circuit paresseux piloté par la fenêtre alpha/bêta :

1. **O(1)**, à partir des compteurs incrémentaux : valeur des trois groupes de
   pièces via une **courbe de rareté concave** (passer de 1 à 2 TZAAR vaut
   énorme, de 10 à 11 presque rien — c'est ce qui encode « perdre tous ses X =
   défaite » de façon continue), plus hauteur et matériel.
2. **Un seul balayage** donnant mobilité et menaces. En feuille, les captures du
   camp au trait viennent d'être générées : seul le camp adverse est balayé,
   soit moitié moins de sondes.

Le terme « étouffement » (l'adversaire n'a plus de capture disponible) est une
condition de victoire à part entière, pas un simple bonus.

Mesure : l'évaluation dynamique coûte ~35 % de vitesse et gagne **81 %** des
parties contre la même recherche sans elle (`--b useDynamic=0`, 16 parties).

---

## Débugger

```bash
make debug                     # ASan + UBSan + assertions + traces
./bin/tzaar_tests_dbg          # la suite complète sous sanitizers
./bin/tzaar_dbg                # le bot avec TRACE (profondeur/score/PV sur stderr)
```

- `TZ_ASSERT` et `check_invariants()` (`-DTZAAR_ASSERT=1`) revérifient après
  chaque coup que les compteurs incrémentaux collent à un recalcul complet.
  C'est le filet qui attrape toute erreur de `do_move`/`undo_move`.
- `board_to_string()` rend le plateau en ASCII, `position_to_input()` produit un
  dump **au format d'entrée CodinGame** : n'importe quelle position rencontrée
  se rejoue et se colle telle quelle.
- Le bot écrit toujours sur stderr `d=<profondeur>/<seldepth> sc=<score>
  n=<nœuds> <ms> pv: <variante>`. Sur CodinGame stderr s'affiche dans la console
  sans polluer le protocole.

---

## Faire évoluer

**Tous les poids sont dans `src/engine/eval_params.hpp`** et accessibles par nom
depuis la ligne de commande — aucune recompilation pour tester une idée :

```bash
./bin/tzaar_arena --list-params
./bin/tzaar_arena --games 40 --time-ms 40 --open 6 --b mobilityWeight=14
./bin/tzaar_arena --games 40 --a useLMP=0 --b lmpBase=2 --b lmpFactor=2
./bin/tzaar_bench --time 40 --set useDynamic=0
```

L'arène rejoue le référé Java à l'identique, alterne les couleurs pour
neutraliser l'avantage du trait, et randomise les N premières demi-actions
(`--open N`) puisque la position de départ est fixe. Elle échoue bruyamment sur
tout coup illégal et signale tout dépassement des 50 ms.

### Ce qui a été mesuré

| Hypothèse | Protocole | Résultat |
|---|---|---|
| L'évaluation dynamique (mobilité, menaces, étouffement) vaut son coût | 16 parties, `--b useDynamic=0` | **81 %** — retenue, malgré −35 % de vitesse |
| PVS + génération par étages + suppression des singletons `static` | bench, budget fixe | 1,0 → **1,4 M nœuds/s** |
| Terme de « course aux pièces rares » (`raceTempo`, `raceEdge`) | 40 parties @20 ms | **47,5 %** — aucun gain, **neutralisé par défaut** |
| Réglages de LMP (`lmpBase`, `lmpFactor`) | bench | profondeur inchangée : le facteur de branchement domine |

Le troisième point mérite d'être détaillé, parce que le constat de départ reste
valable même si le remède a échoué : en self-play les deux moteurs échangent
leurs TZAAR de 6-6 à 1-1 en dix coups. Une évaluation à somme nulle juge en
effet un échange 1 contre 1 neutre, alors qu'il ne l'est pas — arrivé à 1-1,
c'est le trait qui décide. Le terme correctif est implémenté et documenté dans
`eval_params.hpp` mais **désactivé**, faute de gain mesurable. C'est la première
piste à reprendre, avec plus de parties ou d'autres poids.

### Pistes, classées par rapport gain/effort

1. **Régler la courbe de rareté** (`countCurve1..15`) — c'est le cœur de
   l'évaluation, et le paramètre le plus sensible. Une descente de coordonnées
   pilotée par l'arène est directement branchable (`param_names()` expose la
   liste complète).
2. **Protéger les pièces rares par la hauteur** : une pile haute est
   incapturable. Un bonus de hauteur ciblé sur le type le plus rare est le
   concept stratégique de TZAAR que l'évaluation actuelle ne capture pas.
3. **Table d'ouverture** : la position de départ est fixe et le joueur 0 n'a que
   42 premiers coups. Un livre calculé hors ligne est du gain gratuit.
4. **Résolution exacte des finales** : sous ~20 piles la recherche atteint déjà
   des nœuds terminaux ; une TT persistante dédiée prouverait des gains plus tôt.
5. **Pruning des captures** (`capBase`, `capFactor`, désactivé par défaut) :
   la phase 1 propose ~43 captures souvent sans objet. À valider en arène.
6. **Recherche multi-thread** : la structure est monothread et sans état global
   mutable hors `g_params` ; un Lazy SMP serait direct.

### Limite actuelle, honnêtement

La recherche atteint **4 demi-actions, soit 2 tours complets**, en 40 ms. Ce
n'est pas un défaut d'ordonnancement — le taux de coupure sur le premier coup
est de ~99 %, l'arbre est donc quasi minimal. C'est le facteur de branchement
qui plafonne : ≈ 43 captures en phase 1 × ≈ 145 coups en phase 2. Gagner un
tour de profondeur demande ~40× plus de nœuds. Les gains à venir viendront donc
de l'évaluation et du livre d'ouverture, pas de la vitesse brute.

---

## Outils

| Commande | Rôle |
|---|---|
| `bin/tzaar` | le bot, protocole CodinGame sur stdin/stdout |
| `bin/tzaar_tests` | conformité des règles contre l'oracle naïf |
| `bin/tzaar_perft` | comptage d'arbre (non-régression) + débit de la movegen |
| `bin/tzaar_bench` | nœuds/s, profondeur, qualité d'ordonnancement, taux de TT |
| `bin/tzaar_arena` | self-play, A/B de paramètres, contrôle du budget temps |
| `tools/referee.py` | référé complet **via le vrai protocole** : fait jouer deux exécutables quelconques, y compris le Boss Java |

```bash
# valide la couche I/O de bout en bout, et mesure contre le Boss du dépôt
python3 tools/referee.py --p0 ./bin/tzaar --p1 "java -cp ../config Player" -n 10 --swap
```

## Réglages à l'exécution

Variables d'environnement lues par le bot (ignorées sur CodinGame, utiles en
local) : `TZAAR_TIME_MS` (défaut 40, limite plateforme 50), `TZAAR_FIRST_MS`
(défaut 900, limite 1000), `TZAAR_HASH_MB` (défaut 64), `TZAAR_DEPTH`.

Le dépassement mesuré du budget est < 1 ms (contrôle du temps tous les 512
nœuds), et aucune itération n'est entamée au-delà de 60 % du budget.
