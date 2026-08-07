# tools_assets — nettoyage des logos

## Le problème

Les générateurs d'images dessinent souvent le damier de transparence **au lieu**
de produire de la transparence. Le motif se retrouve peint dans les pixels, et
le fichier est parfois un JPEG déguisé en `.png` — donc sans canal alpha du tout.
C'était le cas de `logo2.png` : JPEG 1024×1024, zéro alpha, damier sombre dans
l'image.

Un « supprimer le blanc » ne marche pas : le damier a **deux** couleurs, et le
logo contient lui-même des gris et des reflets clairs.

## L'outil

```bash
pip install pillow scipy
python3 decheckerboard.py entree.png -o sortie.png --trim [--debug]
```

Méthode, en cinq temps :

1. **Grille** — la période est mesurée par régression sur la position sous-pixel
   de chaque front du créneau. Elle est **fractionnaire** : une image
   redimensionnée a une période comme 26,95 px, et un pas entier déphase le
   modèle d'un pixel toutes les ~400 colonnes, ce qui suffit à tout ruiner en
   bord d'image. Chaque axe est ajusté sur ses deux bords, le meilleur résidu
   l'emporte — il suffit qu'un des deux bords soit dégagé du logo.
2. **Motif** — un pixel porte le motif s'il a exactement la couleur attendue
   pour sa case. Un reflet clair du logo peut avoir la bonne teinte, jamais la
   bonne alternance : c'est la densité locale de ce test qui tranche.
3. **Appartenance large** — les pixels **antialiasés entre deux cases** valent la
   moyenne des deux couleurs, donc échouent au test de motif. Ils forment un
   quadrillage de barrières de 2 px qui isole chaque case : sans les admettre,
   toute analyse par connexité échoue. C'était le point dur.
4. **Connexité** — propagation depuis le bord, en **8-connexité** (le bruit du
   canal alpha perce le fond de trous isolés ; en 4-connexité le fond se brise
   en centaines d'îlots). Les contre-formes fermées (intérieur du A, du R) sont
   récupérées si elles portent le motif. Une propagation bornée le long du logo
   efface le liseré résiduel là où la fenêtre de densité est à moitié couverte.
5. **Alpha** — sur les 2-3 px de bord, `P = a·F + (1−a)·B` avec `B` connu donne
   `a`, puis la couleur est décontaminée pour supprimer le halo.

## Réglages

| Option | Effet |
|---|---|
| `--tol` | écart toléré à la couleur de fond. Monter sur une source JPEG bruitée |
| `--neigh` | fraction du voisinage devant porter le motif. **Baisser mord dans le logo** |
| `--grow` | propagation le long du logo. Monter efface le liseré, trop mord les reflets |
| `--min-fg` | aire minimale d'un élément de logo, en pixels |
| `--trim` | recadre sur le contenu |
| `--debug` | écrit aussi le masque et un aperçu sur magenta |

Le taux d'accord affiché en fin d'exécution est un garde-fou : sous 70 % le
modèle de damier est douteux et le résultat mérite un coup d'œil. Il reste
pessimiste quand le logo touche les bords de l'image.

## Résultats

| Sortie | Source | Transparent |
|---|---|---|
| `logo2_transparent.png` (809×585) | `logo2.png`, damier sombre, JPEG | 81 % — accord du modèle 99,3 % |
| `logo_officiel_transparent.png` (839×400) | image jointe, damier clair, alpha bruité | 58 % — `--tol 14 --grow 20` |

Note : `logo_officiel_transparent.png` est le logo commercial officiel de TZAAR
(projet GIPF, Kris Burm). Pour une contribution publique sur CodinGame, votre
propre `logo2_transparent.png` évite la question des droits.

---

# Logo généré par calcul

## Pourquoi pas un fond vert

Un fond vert sert à récupérer de la transparence **qu'on n'a pas**. Dès lors que
l'image est produite par calcul, l'alpha est écrit directement : il n'y a jamais
de fond à retirer, donc rien à incruster. Le fond vert n'aurait résolu aucun
problème ici — il en aurait ajouté un (le vert déteint sur les bords antialiasés
d'un logo métallique, et cette frange se retire moins bien qu'un damier).

## `make_logo.py`

```bash
python3 make_logo.py --out-dir ../src/main/resources/view/assets --width 2048
```

Produit `logo_tzaar.png` (alpha exact) et `logo_tzaar_vert.png` (le même aplati
sur #00FF00, pour votre chaîne de production si vous en avez besoin).

Comment le métal est obtenu — c'est le seul point qui demande une explication :

- **rampe chrome**, pas dégradé. Un métal poli reflète son environnement : ciel
  en haut, horizon éclatant, sol sombre. C'est cette rampe, indexée par la
  **normale de surface**, qui donne la lecture « chrome » ; un simple dégradé
  vertical donne du plastique.
- **biseau par transformée de distance** : la distance au bord de la lettre
  définit un profil en quart de sinus, dont le gradient donne les normales.
- **teinte rouge à gauche, bleue à droite**, discrète : les deux camps du jeu.
- **pions** : les faces supérieures utilisent la même rampe, teintée. Le bord
  *éloigné* reflète l'horizon (clair), le bord proche le sol (sombre) —
  l'inverse donne un pion mat. La pile mixte (corps rouge, sommet bleu) raconte
  la mécanique : capturer, c'est poser sa pièce sur celle de l'adversaire.

Réglages utiles : `--width` (le rendu suréchantillonne ×2, donc 2048 demande
~1,5 Go), `CHROME` pour la rampe, `stacks` pour la composition des piles.

## `chroma_key.py`

Si vous générez une image sur fond uni avec votre propre outil, celui-ci la
détoure :

```bash
python3 chroma_key.py rendu_vert.png -o logo.png --key 00FF00 --trim
```

Deux pièges qu'il traite et qu'un seuillage simple laisse passer :

1. **débordement de teinte** — sur les bords antialiasés le pixel est un mélange
   du logo et du vert ; on résout `P = a·F + (1−a)·V` pour retrouver alpha, puis
   on retire la part de vert de la couleur ;
2. **noir sous l'alpha** — mettre à zéro le RGB des pixels transparents paraît
   propre, mais tout moteur de rendu interpole RGB et alpha séparément et ce
   noir remonte dans les contours. La couleur du bord est donc recopiée dans
   toute la zone transparente.

Vérifié en boucle : `logo_tzaar_vert.png` repassé dans `chroma_key.py` redonne
bien un détourage propre.
