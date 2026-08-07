#!/usr/bin/env python3
"""
Retire un damier "fausse transparence" et le remplace par un vrai canal alpha.

Beaucoup de generateurs d'images dessinent le damier de transparence AU LIEU de
produire de la transparence : le motif se retrouve peint dans les pixels (et le
fichier est parfois un JPEG deguise en .png). Un simple "supprimer le blanc" ne
marche pas, parce que le damier a deux couleurs et que le logo contient lui-meme
des gris et des reflets clairs.

Methode :
  1. la maille du damier est retrouvee par autocorrelation sur la bande de bord ;
  2. on en deduit une image de fond attendue B(x,y) (les deux couleurs alternees) ;
  3. un pixel est du fond s'il colle a B ET si son VOISINAGE y colle aussi
     (le motif alterne est une signature qu'un reflet du logo ne reproduit pas) ;
  4. la connexite au bord evite de trouer le logo ; les contre-formes fermees
     (interieur du A, du R...) sont recuperees si elles portent le motif ;
  5. sur les 2-3 px de bord antialiase, alpha est estime par
     P = a*F + (1-a)*B, puis la couleur est decontaminee pour supprimer le halo.

  python3 decheckerboard.py entree.png -o sortie.png [--debug]
"""
import argparse, sys
import numpy as np
from PIL import Image
from scipy import ndimage


def _frame(usable, border):
    """Cadre de `border` px le long des quatre bords : forcement du fond, sauf
    la ou le logo touche les bords."""
    f = usable.copy()
    f[border:-border, border:-border] = False
    return f


def _mad(v):
    return float(np.median(np.abs(v - np.median(v))))


def _mode(v):
    """Valeur la plus frequente. Sur le cadre de bord, le fond est toujours
    majoritaire, meme quand le logo touche un bord : la mediane s'y laisse
    entrainer, le mode non."""
    h = np.bincount(np.clip(np.round(v), 0, 255).astype(int), minlength=256)
    return float(np.argmax(h))


def _profile(lum, keep, border, fin=False):
    """Profil 1D le long de l'axe 1, sur une bande de `border` lignes prise en
    haut (ou en bas si `fin`). La bande tient dans UNE rangee de cases, donc le
    profil est un creneau propre de periode 2*maille.

    On prend la MEDIANE, et seulement sur les pixels retenus par `keep` (gris,
    opaques) : la ou le logo mord sur la bande de bord, une moyenne se ferait
    entrainer alors qu'une mediane tient, et les colonnes trop entamees sont
    simplement declarees invalides."""
    band = lum[-border:] if fin else lum[:border]
    kb = keep[-border:] if fin else keep[:border]
    v = np.where(kb, band, np.nan)
    with np.errstate(invalid="ignore"):
        sig = np.nanmedian(v, axis=0)
    ok = kb.sum(0) >= max(3, border // 3)
    return np.nan_to_num(sig), ok


def _fit_axis(sig, ok, approx):
    """Pas et decalage d'un axe, par REGRESSION sur les transitions.

    Une image redimensionnee n'a plus une periode entiere (ici 26,93 px pour 27
    verticalement) : avec un pas entier la phase derive d'un pixel toutes les
    ~400 colonnes et le modele de fond se decale completement au bord.

    Plutot que de balayer des pas reels au juge, on releve la position
    sous-pixel de chaque front du creneau, on leur attribue un indice de case
    par arrondi, et on ajuste t = o + k*c au sens des moindres carres. Les
    fronts manquants (la ou le logo recouvre le damier) ne genent pas, et les
    aberrants sont ecartes par un second passage.
    """
    n = len(sig)
    v = np.where(ok, sig, np.nan)
    fini = v[np.isfinite(v)]
    if len(fini) < 20:
        return float(approx), 0.0, 1e9
    lo = _mode(fini[fini <= np.median(fini)])
    hi = _mode(fini[fini >= np.median(fini)])
    if abs(hi - lo) < 12:
        return float(approx), 0.0, 1e9
    mid = 0.5 * (lo + hi)

    # fronts sous-pixel entre deux echantillons valides consecutifs
    t = []
    for i in range(n - 1):
        if not (ok[i] and ok[i + 1]):
            continue
        a_, b_ = sig[i], sig[i + 1]
        if (a_ - mid) * (b_ - mid) < 0 and abs(b_ - a_) > 8:
            t.append(i + (mid - a_) / (b_ - a_))
    if len(t) < 4:
        return float(approx), 0.0, 1e9
    t = np.array(t)

    c, o = float(approx), t[0]
    for _ in range(3):
        k = np.round((t - o) / c)
        A = np.stack([np.ones_like(k), k], 1)
        sol, *_ = np.linalg.lstsq(A, t, rcond=None)
        o, c = float(sol[0]), float(sol[1])
        res = np.abs(t - (o + k * c))
        garde = res < max(2.5, 3 * np.median(res))
        if garde.sum() >= 4:
            t = t[garde]
    k = np.round((t - o) / c)
    residu = float(np.median(np.abs(t - (o + k * c)))) if len(t) else 1e9
    # un pas aberrant vaut mieux etre rejete que subi
    if not (0.55 * approx < c < 1.8 * approx):
        return float(approx), 0.0, 1e9
    return c, o % c, residu


def detect_grid(lum, keep, border=18):
    """Pas et decalage du damier, un axe a la fois (une recherche 2D serait a la
    fois plus lente et plus fragile). Les pas sont reels, pas entiers.

    Chaque axe est estime sur SES DEUX bords, et on garde l'ajustement dont les
    fronts sont les plus reguliers : il suffit que l'un des deux bords soit
    degage du logo."""
    approx = None
    for L, K in ((lum, keep), (lum.T, keep.T)):
        sig, ok = _profile(L, K, border)
        c = sig.copy(); c[~ok] = 0
        if ok.sum() > 10:
            c[ok] -= sig[ok].mean()
        ac = np.correlate(c, c, "full")[len(c) - 1:]
        ac = ac / max(ac[0], 1e-9)
        k = int(np.argmin(ac[6:min(160, len(ac))])) + 6   # creux = une maille
        approx = k if approx is None else min(approx, k)

    def meilleur(L, K):
        out = []
        for fin in (False, True):
            # Le profil court le long de l'AUTRE axe : changer de bande change
            # les lignes moyennees, pas l'origine des abscisses. Aucune
            # correction de decalage n'est donc a appliquer ici.
            sig, ok = _profile(L, K, border, fin)
            c, o, r = _fit_axis(sig, ok, approx)
            out.append((r, c, o, fin))
        out.sort()
        return out[0]

    rx, cx, ox, fx = meilleur(lum, keep)
    ry, cy, oy, fy = meilleur(lum.T, keep.T)
    print(f"  ajustement X : pas {cx:.3f} decalage {ox:.2f} residu {rx:.2f} px "
          f"(bord {'bas' if fx else 'haut'})")
    print(f"  ajustement Y : pas {cy:.3f} decalage {oy:.2f} residu {ry:.2f} px "
          f"(bord {'droit' if fy else 'gauche'})")
    return (cy, cx), (oy, ox)


def parity(shape, cell, off):
    H, W = shape
    cy, cx = cell
    oy, ox = off
    iy = np.floor((np.arange(H) - oy) / cy).astype(int)[:, None]
    ix = np.floor((np.arange(W) - ox) / cx).astype(int)[None, :]
    return (iy + ix) % 2


def build_background(shape, cell, off, lum, usable, border=16):
    par = parity(shape, cell, off)
    edge = _frame(usable, border)
    c0 = _mode(lum[edge & (par == 0)])
    c1 = _mode(lum[edge & (par == 1)])
    return np.where(par == 0, c0, c1), (c0, c1), par


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("--tol", type=float, default=12.0,
                    help="ecart tolere a la couleur de fond attendue")
    ap.add_argument("--sat", type=float, default=26.0,
                    help="saturation maximale d'un pixel de fond (le damier est gris)")
    ap.add_argument("--neigh", type=float, default=0.55,
                    help="fraction du voisinage 11x11 devant porter le motif")
    ap.add_argument("--grow", type=int, default=16,
                    help="propagation du fond le long du logo, en pixels")
    ap.add_argument("--min-fg", type=int, default=150,
                    help="aire minimale d'une composante de logo, en pixels")
    ap.add_argument("--no-bleed", action="store_true",
                    help="ne pas propager la couleur du bord dans la zone transparente")
    ap.add_argument("--trim", action="store_true", help="recadrer sur le contenu")
    ap.add_argument("--debug", action="store_true")
    a = ap.parse_args()

    im = Image.open(a.input).convert("RGBA")
    arr = np.array(im).astype(np.float64)
    rgb, al = arr[..., :3], arr[..., 3]
    H, W = al.shape
    lum = rgb.mean(2)
    sat = rgb.max(2) - rgb.min(2)
    # les pixels deja transparents ne portent aucune information exploitable
    usable = al > 200
    already_clear = al < 40

    # Seuls les pixels gris et opaques servent a mesurer la grille : le damier
    # est achromatique, le logo ne l'est pas.
    gris = usable & (sat < 34)
    cell, off = detect_grid(lum, gris)
    B, (c0, c1), par = build_background((H, W), cell, off, lum, usable)
    print(f"maille detectee : {cell[1]:.3f} x {cell[0]:.3f} px, "
          f"decalage ({off[1]:.1f}, {off[0]:.1f})")
    print(f"couleurs du damier : {c0:.0f} et {c1:.0f}")
    # Garde-fou : le modele doit expliquer les pixels GRIS du cadre de bord.
    # (Les pixels colores du cadre appartiennent au logo quand celui-ci touche
    # un bord ; les compter ferait crier au loup a chaque image.)
    fr = _frame(usable, 16) & (sat < 34)
    accord = (np.abs(lum - B)[fr] < 14).mean() * 100 if fr.any() else 0.0
    print(f"accord du modele sur les pixels gris du bord : {accord:.1f}%")
    if accord < 70:
        print("  ATTENTION : modele de damier peu fiable, resultat a verifier")

    # --- 1. pixels compatibles avec le fond ---------------------------------
    # (a) preuve de MOTIF : le pixel a exactement la couleur attendue pour sa
    #     case. Les pixels deja transparents comptent, sinon ils creusent la
    #     densite de voisinage et font rejeter le damier autour d'eux.
    motif = ((np.abs(lum - B) < a.tol) & (sat < a.sat) & usable) | already_clear

    # (b) appartenance possible au fond, plus large. Indispensable : les pixels
    #     ANTIALIASES entre deux cases valent ~225, a mi-chemin des deux
    #     couleurs, donc a plus de `tol` de chacune. Ils forment un quadrillage
    #     de barrieres de 2 px qui isole chaque case du damier et fait echouer
    #     toute analyse par connexite. On les admet ici.
    lo, hi = (c0, c1) if c0 <= c1 else (c1, c0)
    large = ((lum > lo - a.tol) & (lum < hi + a.tol) & (sat < a.sat) & usable) | already_clear

    # (c) un reflet clair du logo peut avoir la bonne teinte, jamais la bonne
    #     alternance : c'est la densite locale de (a) qui tranche.
    dens = ndimage.uniform_filter(motif.astype(np.float64), 11)
    bgcand = (large & (dens > a.neigh)) | already_clear

    # --- 2. connexite au bord ------------------------------------------------
    # 8-connexite IMPERATIVE : le bruit du canal alpha perce le fond de trous
    # isoles, et en 4-connexite le fond se retrouve casse en centaines d'ilots
    # dont la plupart ne touchent plus le bord.
    VOIS8 = np.ones((3, 3), bool)
    lab, n = ndimage.label(bgcand, structure=VOIS8)
    border_labels = set(lab[0].tolist()) | set(lab[-1].tolist()) | \
                    set(lab[:, 0].tolist()) | set(lab[:, -1].tolist())
    border_labels.discard(0)
    bg = np.isin(lab, list(border_labels))

    # --- 3. contre-formes fermees (interieur du A, du R...) ------------------
    # On les recupere si elles sont assez grandes ET portent les deux parites,
    # ce qui distingue un vrai trou de fond d'un simple reflet.
    for i in range(1, n + 1):
        if i in border_labels:
            continue
        m = lab == i
        if m.sum() < 3 * cell[0]:
            continue
        p = par[m]
        if p.min() != p.max():          # la region traverse une frontiere de case
            bg |= m
    # --- 3 ter. frange le long du logo ---------------------------------------
    # Pres du logo, la fenetre de densite 11x11 est a moitie couverte par le
    # logo : le test de motif y echoue et il subsiste un lisere de damier
    # collé aux lettres. On propage donc le fond dans les pixels compatibles
    # voisins, mais sur une distance BORNEE, pour ne pas risquer de fuir dans
    # un reflet clair du logo.
    for _ in range(a.grow):
        bg = ndimage.binary_dilation(bg, np.ones((3, 3))) & (large | bg)

    bg = ndimage.binary_closing(bg, np.ones((3, 3)))
    bg = ndimage.binary_opening(bg, np.ones((3, 3)))

    # --- 3 bis. mouchetures ---------------------------------------------------
    # Les images recuperees sont souvent bruitees : des points sombres trainent
    # sur le damier. Un simple filtre par taille ne suffit pas, parce que ces
    # points se touchent en diagonale et forment une toile connectee au logo.
    # On ouvre donc d'abord (ce qui rompt les liens d'un pixel), on garde les
    # composantes larges, puis on redilate DANS le masque d'origine pour
    # rendre au logo le pixel de bord mange par l'ouverture.
    fg = ~bg
    core = ndimage.binary_opening(fg, np.ones((3, 3)))
    lab2, n2 = ndimage.label(core, structure=VOIS8)
    if n2:
        sizes = ndimage.sum(np.ones_like(lab2), lab2, range(1, n2 + 1))
        gardes = np.nonzero(sizes >= a.min_fg)[0] + 1
        core = np.isin(lab2, gardes)
        print(f"{n2 - len(gardes)} mouchetures retirees sur {n2} composantes "
              f"(la plus grosse : {int(sizes.max())} px)")
    for _ in range(2):
        core = ndimage.binary_dilation(core, np.ones((3, 3))) & fg
    bg = ~core

    # --- 4. alpha sur la bande antialiasee -----------------------------------
    solid_fg = ~ndimage.binary_dilation(bg, np.ones((7, 7)))
    band = (~bg) & (~solid_fg)

    alpha = np.where(bg, 0.0, 255.0)
    if band.any():
        # couleur de premier plan la plus proche, pour calibrer le melange
        idx = ndimage.distance_transform_edt(~solid_fg, return_distances=False,
                                             return_indices=True)
        Fnear = rgb[idx[0], idx[1]]
        Bc = np.stack([B, B, B], -1)
        num = ((rgb - Bc) * (Fnear - Bc)).sum(-1)
        den = ((Fnear - Bc) ** 2).sum(-1)
        est = np.ones_like(den)
        np.divide(num, den, out=est, where=den > 1e-6)
        est = np.clip(est, 0.0, 1.0)
        alpha = np.where(band, est * 255.0, alpha)
        # decontamination : on retire la part de fond melangee au bord
        with np.errstate(invalid="ignore", divide="ignore"):
            F = (rgb - (1 - est[..., None]) * Bc) / np.maximum(est[..., None], 1e-3)
        rgb = np.where((band & (est > 0.02))[..., None], np.clip(F, 0, 255), rgb)

    # --- 5. debordement de couleur (color bleed) -----------------------------
    # Mettre les pixels transparents a NOIR est le piege classique : des que le
    # moteur de rendu filtre l'image (mipmaps, mise a l'echelle, rotation), il
    # interpole RGB et alpha separement, et ce noir remonte dans les bords ->
    # lisere sombre autour du logo. On recopie donc dans toute la zone
    # transparente la couleur du pixel opaque le plus proche. Invisible tel
    # quel (alpha vaut 0), mais le filtrage n'a plus rien de sombre a aspirer.
    if not a.no_bleed:
        opaque_mask = alpha > 0
        if opaque_mask.any() and (~opaque_mask).any():
            idx = ndimage.distance_transform_edt(~opaque_mask, return_distances=False,
                                                 return_indices=True)
            proche = rgb[idx[0], idx[1]]
            rgb = np.where(opaque_mask[..., None], rgb, proche)

    out = np.dstack([rgb, alpha]).clip(0, 255).astype(np.uint8)
    img = Image.fromarray(out, "RGBA")

    if a.trim:
        bb = img.getbbox()
        if bb:
            img = img.crop(bb)

    img.save(a.output)
    op = (alpha > 250).mean() * 100
    tr = (alpha < 5).mean() * 100
    print(f"ecrit {a.output} : {img.size[0]}x{img.size[1]}, "
          f"{tr:.1f}% transparent, {op:.1f}% opaque, {100-op-tr:.1f}% partiel")

    if a.debug:
        Image.fromarray((bg * 255).astype(np.uint8)).save(a.output + ".masque.png")
        chk = Image.new("RGBA", img.size, (255, 0, 255, 255))
        Image.alpha_composite(chk, img).convert("RGB").save(a.output + ".magenta.png")


if __name__ == "__main__":
    main()
