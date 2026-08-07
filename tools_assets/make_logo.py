#!/usr/bin/env python3
"""
Genere le logo TZAAR entierement par calcul : metal chrome, biseaux, piles de
pions. Rien n'est peint a la main, donc le canal alpha est EXACT par
construction -- il n'y a jamais de fond a detourer.

Deux sorties :
  logo_tzaar.png        transparence reelle (celle a utiliser)
  logo_tzaar_vert.png   le meme aplati sur vert fluo #00FF00

Rendu en suréchantillonnage puis reduit : les bords sont propres sans avoir a
antialiaser quoi que ce soit a la main.

  python3 make_logo.py --out-dir ../src/main/resources/view/assets [--width 2048]
"""
import argparse, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

FONT = "/usr/share/fonts/truetype/lato/Lato-Black.ttf"
FONT_LIGHT = "/usr/share/fonts/truetype/lato/Lato-Medium.ttf"

# --- rampe chrome ----------------------------------------------------------
# Un metal poli reflete son environnement : ciel clair en haut, horizon
# eclatant, sol sombre en bas. C'est cette rampe, et non un simple degrade,
# qui donne la lecture "chrome".
CHROME = [
    (0.00, (14, 18, 28)), (0.14, (58, 68, 88)), (0.30, (140, 154, 176)),
    (0.44, (232, 240, 250)), (0.52, (255, 255, 255)), (0.60, (128, 140, 162)),
    (0.72, (44, 52, 70)), (0.84, (108, 120, 142)), (0.93, (196, 208, 226)),
    (1.00, (150, 162, 184)),
]


def ramp_lut(stops, n=1024):
    lut = np.zeros((n, 3), np.float32)
    pos = np.array([s[0] for s in stops])
    col = np.array([s[1] for s in stops], float)
    x = np.linspace(0, 1, n)
    for c in range(3):
        lut[:, c] = np.interp(x, pos, col[:, c])
    return lut


def bevel_normals(mask, width):
    """Normales de surface d'un biseau chanfreine, deduites de la distance au
    bord : le profil monte en quart de sinus puis reste plat au centre."""
    d = ndimage.distance_transform_edt(mask > 0.5)
    h = np.clip(d / max(width, 1e-6), 0, 1)
    h = np.sin(h * np.pi / 2) ** 0.9
    h = ndimage.gaussian_filter(h, width * 0.16)
    gy, gx = np.gradient(h * width * 0.85)
    nz = np.ones_like(gx)
    n = np.sqrt(gx ** 2 + gy ** 2 + nz ** 2)
    return -gx / n, -gy / n, nz / n, h


def metal_fill(mask, lut, tint=None, bevel=None, spec_power=48, spec_gain=1.0,
               vertical=0.55, seed=7):
    """Remplit `mask` d'un metal : rampe indexee par la normale ET par la
    position verticale, plus un speculaire et un fin brossage."""
    H, W = mask.shape
    ys, xs = np.nonzero(mask > 0.02)
    if len(ys) == 0:
        return np.zeros((H, W, 3))
    y0, y1 = ys.min(), ys.max()
    bw = bevel if bevel is not None else max(2.0, (y1 - y0) * 0.09)
    nx, ny, nz, h = bevel_normals(mask, bw)

    yfrac = np.clip((np.arange(H) - y0) / max(y1 - y0, 1), 0, 1)[:, None]
    yfrac = np.repeat(yfrac, W, axis=1)

    t = vertical * yfrac + (1 - vertical) * (0.5 - 0.5 * ny)
    # brossage : un bruit tres etire horizontalement, presque subliminal
    rng = np.random.default_rng(seed)
    brush = ndimage.gaussian_filter(rng.standard_normal((H, W)), (0.4, 9.0))
    t = np.clip(t + brush * 0.05, 0, 1)

    idx = (t * (len(lut) - 1)).astype(int)
    col = lut[idx]

    # speculaire : lumiere haute-gauche
    lx, ly, lz = -0.42, -0.72, 0.55
    ln = np.sqrt(lx * lx + ly * ly + lz * lz)
    lx, ly, lz = lx / ln, ly / ln, lz / ln
    hx, hy, hz = lx, ly, lz + 1.0
    hn = np.sqrt(hx * hx + hy * hy + hz * hz)
    ndoth = np.clip((nx * hx + ny * hy + nz * hz) / hn, 0, 1)
    col += (ndoth ** spec_power)[..., None] * 255.0 * spec_gain

    if tint is not None:
        col = col * (1 - tint[2]) + tint[2] * col * tint[0][None, None, :] / 128.0
    return np.clip(col, 0, 255)


def fit_size(text, target_w, font_path, tracking_ratio):
    """Plus grand corps tel que le mot, interlettrage compris, tienne dans
    `target_w`. Une taille devinee finit toujours par deborder du cadre."""
    lo, hi = 8, 4000
    while lo < hi:
        mid = (lo + hi + 1) // 2
        f = ImageFont.truetype(font_path, mid)
        w = sum(f.getlength(c) for c in text) + tracking_ratio * mid * (len(text) - 1)
        if w <= target_w:
            lo = mid
        else:
            hi = mid - 1
    return lo


def text_mask(text, canvas, target_w, tracking_ratio=0.06, font_path=FONT):
    """Masque d'un texte ajuste a une largeur cible, avec interlettrage (une
    police de logo respire beaucoup plus qu'un texte courant)."""
    W, H = canvas
    size = fit_size(text, target_w, font_path, tracking_ratio)
    font = ImageFont.truetype(font_path, size)
    tracking = tracking_ratio * size
    total = sum(font.getlength(c) for c in text) + tracking * (len(text) - 1)
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    x = (W - total) / 2
    asc, desc = font.getmetrics()
    y = (H - (asc + desc)) / 2
    for ch in text:
        d.text((x, y), ch, font=font, fill=255)
        x += font.getlength(ch) + tracking
    return np.array(img).astype(float) / 255.0


def place(mask, cx, cy):
    """Recentre un masque sur (cx, cy) d'apres sa boite englobante reelle, et
    non d'apres la boite de la police : c'est l'encre qui doit etre centree."""
    ys, xs = np.nonzero(mask > 0.02)
    if len(ys) == 0:
        return mask
    my, mx = (ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2
    return np.roll(np.roll(mask, int(round(cy - my)), 0), int(round(cx - mx)), 1)


def over(dst, src):
    """Composition alpha classique, en premultiplie pour eviter les franges."""
    sa = src[..., 3:4]
    da = dst[..., 3:4]
    out_a = sa + da * (1 - sa)
    safe = np.maximum(out_a, 1e-6)
    out_rgb = (src[..., :3] * sa + dst[..., :3] * da * (1 - sa)) / safe
    return np.concatenate([out_rgb, out_a], axis=-1)


def layer(rgb, alpha):
    return np.concatenate([np.asarray(rgb, np.float32),
                           np.asarray(alpha, np.float32)[..., None]], axis=-1)


def shadow(mask, blur, dy, dx, strength, color=(0, 0, 0)):
    a = ndimage.gaussian_filter(np.roll(np.roll(mask, dy, 0), dx, 1), blur) * strength
    rgb = np.zeros(mask.shape + (3,), np.float32)
    rgb[..., 0], rgb[..., 1], rgb[..., 2] = color
    return layer(rgb, np.clip(a, 0, 1))


def ellipse_mask(shape, cx, cy, rx, ry, ss=1):
    # float32 partout : en float64 un seul tampon RGBA pleine toile depasse
    # 200 Mo a 1600 px de large, et le rendu se fait tuer par le systeme.
    H, W = shape
    y = ((np.arange(H, dtype=np.float32) - cy) / ry)[:, None]
    x = ((np.arange(W, dtype=np.float32) - cx) / rx)[None, :]
    r = np.sqrt(x * x + y * y)
    return np.clip((1.0 - r) * max(rx, ry) * 0.5, 0, 1).astype(np.float32), r


def metal_ramp(base, n=1024):
    """Rampe de metal COLORE. Un metal peint n'est pas une couleur eclaircie :
    il garde la structure du chrome (bande claire a l'horizon, sol sombre) et
    ne fait que la teinter. C'est ce qui distingue le metal du plastique."""
    b = np.array(base, float)
    blanc = np.array([255, 255, 255], float)
    stops = [
        (0.00, b * 0.14),
        (0.16, b * 0.42),
        (0.34, b * 0.92),
        (0.46, b * 0.75 + blanc * 0.42),
        (0.53, b * 0.30 + blanc * 0.78),
        (0.62, b * 0.85),
        (0.74, b * 0.30),
        (0.86, b * 0.62),
        (1.00, b * 0.34 + blanc * 0.12),
    ]
    lut = np.zeros((n, 3), np.float32)
    pos = np.array([p for p, _ in stops])
    col = np.clip(np.array([c for _, c in stops]), 0, 255)
    x = np.linspace(0, 1, n)
    for c in range(3):
        lut[:, c] = np.interp(x, pos, col[:, c])
    return lut


def draw_stack(shape, cx, base_y, rx, ry, thickness, count, base_col, top_col):
    """Une pile de pions vue en legere plongee. Chaque pion est un cylindre :
    paroi laterale eclairee par l'angle azimutal, face superieure bombee dont
    la normale indexe la rampe de metal, et un jonc argente sur l'arete."""
    H, W = shape
    out = np.zeros((H, W, 4), np.float32)
    xs = (np.arange(W) - cx)[None, :] / rx
    for i in range(count):
        cy = base_y - i * thickness
        col = top_col if i == count - 1 else base_col
        lut = metal_ramp(col)

        # --- paroi laterale : cylindre, donc t depend de l'angle azimutal ----
        wall = np.zeros((H, W), np.float32)
        for k in range(int(thickness) + 1):
            m, _ = ellipse_mask((H, W), cx, cy + k, rx, ry)
            wall = np.maximum(wall, m)
        top, r = ellipse_mask((H, W), cx, cy, rx, ry)
        wall = np.clip(wall - top, 0, 1)
        # Cylindre : la normale vaut (u, 0, sqrt(1-u2)). Lumiere en haut a
        # gauche -> clair a gauche, sombre a droite.
        u = np.clip(xs, -1, 1)
        z = np.sqrt(np.clip(1 - u * u, 0, 1))
        lambert = np.clip(0.22 + 0.78 * (-0.55 * u + 0.83 * z), 0, 1.15)
        t_wall = np.clip(0.82 - 0.44 * lambert, 0.02, 0.98) + np.zeros((H, 1))
        wall_rgb = lut[(t_wall * (len(lut) - 1)).astype(int)]
        out = over(out, layer(wall_rgb, wall))

        # --- face superieure : dome doux -------------------------------------
        rr = np.clip(r, 0, 1)
        dome = np.sqrt(np.clip(1 - rr ** 2, 0, 1))
        # Bornees a [-1,1] : hors de l'ellipse ces "normales" divergent et
        # ramenaient l'index de rampe dans une zone sombre, ce qui noircissait
        # le bord eloigne des pions au lieu de l'eclairer.
        ny_ = np.clip(-(np.arange(H) - cy)[:, None] / ry, -1, 1)
        nx_ = np.clip((np.arange(W) - cx)[None, :] / rx, -1, 1)
        # Face tournee vers le ciel : le bord ELOIGNE reflete l'horizon (clair),
        # le bord proche reflete le sol (sombre). L'inverse donne un pion mat.
        t_top = np.clip(0.47 + 0.17 * (1 - ny_) - 0.06 * nx_, 0.02, 0.98)
        face = lut[(t_top * (len(lut) - 1)).astype(int)]
        # tache speculaire en haut a gauche
        spec = np.exp(-(((nx_ + 0.42) ** 2 + (ny_ - 0.46) ** 2) / 0.10))
        face += (spec * 120)[..., None]
        # anneau grave, signature des pions du jeu
        face += (np.exp(-((rr - 0.72) ** 2) / 0.0012) * 40)[..., None]
        # jonc argente sur l'arete
        jonc = np.exp(-((rr - 0.955) ** 2) / 0.0009)
        face = face * (1 - jonc * 0.65)[..., None] + (jonc * 190)[..., None]
        out = over(out, layer(np.clip(face, 0, 255), top))

        # --- rainure entre deux pions ----------------------------------------
        if i < count - 1:
            sep, _ = ellipse_mask((H, W), cx, cy - thickness + 1, rx, ry)
            out = over(out, layer(np.zeros((H, W, 3)),
                                  np.clip(sep - ellipse_mask((H, W), cx, cy - thickness + 1,
                                                             rx * 0.995, ry * 0.99)[0], 0, 1) * 0.5))
    return out


def fit_size(text, target_w, font_path, tracking_ratio):
    """Plus grand corps tel que le mot, interlettrage compris, tienne dans
    `target_w`. Une taille devinee finit toujours par deborder du cadre."""
    lo, hi = 8, 4000
    while lo < hi:
        mid = (lo + hi + 1) // 2
        f = ImageFont.truetype(font_path, mid)
        w = sum(f.getlength(c) for c in text) + tracking_ratio * mid * (len(text) - 1)
        if w <= target_w:
            lo = mid
        else:
            hi = mid - 1
    return lo


def text_mask(text, canvas, target_w, tracking_ratio=0.06, font_path=FONT):
    """Masque d'un texte ajuste a une largeur cible, avec interlettrage (une
    police de logo respire beaucoup plus qu'un texte courant)."""
    W, H = canvas
    size = fit_size(text, target_w, font_path, tracking_ratio)
    font = ImageFont.truetype(font_path, size)
    tracking = tracking_ratio * size
    total = sum(font.getlength(c) for c in text) + tracking * (len(text) - 1)
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    x = (W - total) / 2
    asc, desc = font.getmetrics()
    y = (H - (asc + desc)) / 2
    for ch in text:
        d.text((x, y), ch, font=font, fill=255)
        x += font.getlength(ch) + tracking
    return np.array(img).astype(float) / 255.0


def place(mask, cx, cy):
    """Recentre un masque sur (cx, cy) d'apres sa boite englobante reelle, et
    non d'apres la boite de la police : c'est l'encre qui doit etre centree."""
    ys, xs = np.nonzero(mask > 0.02)
    if len(ys) == 0:
        return mask
    my, mx = (ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2
    return np.roll(np.roll(mask, int(round(cy - my)), 0), int(round(cx - mx)), 1)


def over(dst, src):
    """Composition alpha classique, en premultiplie pour eviter les franges."""
    sa = src[..., 3:4]
    da = dst[..., 3:4]
    out_a = sa + da * (1 - sa)
    safe = np.maximum(out_a, 1e-6)
    out_rgb = (src[..., :3] * sa + dst[..., :3] * da * (1 - sa)) / safe
    return np.concatenate([out_rgb, out_a], axis=-1)


def layer(rgb, alpha):
    return np.concatenate([np.asarray(rgb, np.float32),
                           np.asarray(alpha, np.float32)[..., None]], axis=-1)


def shadow(mask, blur, dy, dx, strength, color=(0, 0, 0)):
    a = ndimage.gaussian_filter(np.roll(np.roll(mask, dy, 0), dx, 1), blur) * strength
    rgb = np.zeros(mask.shape + (3,), np.float32)
    rgb[..., 0], rgb[..., 1], rgb[..., 2] = color
    return layer(rgb, np.clip(a, 0, 1))


def ellipse_mask(shape, cx, cy, rx, ry, ss=1):
    # float32 partout : en float64 un seul tampon RGBA pleine toile depasse
    # 200 Mo a 1600 px de large, et le rendu se fait tuer par le systeme.
    H, W = shape
    y = ((np.arange(H, dtype=np.float32) - cy) / ry)[:, None]
    x = ((np.arange(W, dtype=np.float32) - cx) / rx)[None, :]
    r = np.sqrt(x * x + y * y)
    return np.clip((1.0 - r) * max(rx, ry) * 0.5, 0, 1).astype(np.float32), r


def build(width=2048):
    SS = 2                                   # surechantillonnage
    W, H = width * SS, int(width * 0.62) * SS
    lut = ramp_lut(CHROME)
    canvas = np.zeros((H, W, 4), np.float32)

    # ---- mise en page : trois bandes, calculees et non devinees -------------
    #   emblem  : les piles de pions, posees sur une ligne de sol
    #   mot     : TZAAR, occupe 90% de la largeur utile
    #   baseline: le sous-titre
    SOL = 0.330 * H            # ligne de sol des piles
    CY_MOT = 0.655 * H
    CY_SUB = 0.915 * H

    # ---------------------------------------------------------------- pions --
    ry = 0.0275 * W
    rx = ry * 2.30
    th = ry * 0.80
    ROUGE = (198, 40, 48)
    BLEU = (38, 96, 200)
    # trois piles : la mixte (corps rouge, sommet bleu) raconte la mecanique du
    # jeu -- capturer, c'est poser sa piece SUR celle de l'adversaire.
    stacks = [(-1.18, 3, ROUGE, ROUGE), (0.0, 5, BLEU, BLEU), (1.18, 2, ROUGE, BLEU)]
    piles = np.zeros((H, W, 4), np.float32)
    for dx, n, cb, ct in stacks:
        cx = W / 2 + dx * rx
        piles = over(piles, draw_stack((H, W), cx, SOL, rx, ry, th, n, cb, ct))
    # ombre portee au sol, ecrasee verticalement
    sol, _ = ellipse_mask((H, W), W / 2, SOL + ry * 0.75, rx * 2.9, ry * 0.70)
    canvas = over(canvas, layer(np.zeros((H, W, 3)),
                                ndimage.gaussian_filter(sol, ry * 0.45) * 0.55))
    canvas = over(canvas, piles)

    # --------------------------------------------------------------- TZAAR ---
    m = place(text_mask("TZAAR", (W, H), 0.90 * W, 0.055), W / 2, CY_MOT)

    d = ndimage.distance_transform_edt(m < 0.5)
    contour_ext = np.clip((0.0085 * W - d) / 2.0, 0, 1)
    contour_int = np.clip((0.0040 * W - d) / 2.0, 0, 1)

    canvas = over(canvas, shadow(contour_ext, 0.020 * W, int(0.018 * W), 0, 0.66))
    canvas = over(canvas, layer(np.zeros((H, W, 3), np.float32) + np.array([9, 11, 18]), contour_ext))
    canvas = over(canvas, layer(np.zeros((H, W, 3), np.float32) + np.array([132, 144, 168]), contour_int))

    # teinte : rouge a gauche, bleu a droite -- les deux camps du jeu
    grad = np.clip((np.arange(W) / W - 0.5) * 2.4, -1, 1)[None, :]
    tint_rgb = np.zeros((H, W, 3), np.float32)
    tint_rgb[..., 0] = 138 - 28 * grad
    tint_rgb[..., 1] = 128
    tint_rgb[..., 2] = 138 + 32 * grad
    metal = metal_fill(m, lut, bevel=0.017 * W, spec_power=40, vertical=0.60)
    metal = metal * (0.78 + 0.22 * tint_rgb / 128.0)
    canvas = over(canvas, layer(np.clip(metal, 0, 255), m))

    # ------------------------------------------------------------- baseline --
    sub = place(text_mask("AN ABSTRACT STRATEGY GAME", (W, H), 0.54 * W, 0.30,
                          font_path=FONT_LIGHT), W / 2, CY_SUB)
    canvas = over(canvas, shadow(sub, 0.003 * W, int(0.0025 * W), 0, 0.6))
    canvas = over(canvas, layer(np.zeros((H, W, 3), np.float32) + np.array([216, 224, 238]), sub))

    # ------------------------------------------------------------ reduction --
    img = Image.fromarray(np.clip(canvas * np.array([1, 1, 1, 255]), 0, 255).astype(np.uint8), "RGBA")
    return img.resize((W // SS, H // SS), Image.LANCZOS)


def bleed(img):
    """Recopie la couleur du bord dans la zone transparente : sans ca, le
    filtrage d'un moteur de rendu aspire du noir dans les contours."""
    a = np.array(img).astype(float)
    op = a[..., 3] > 0
    if op.any() and (~op).any():
        idx = ndimage.distance_transform_edt(~op, return_distances=False, return_indices=True)
        a[..., :3] = np.where(op[..., None], a[..., :3], a[..., :3][idx[0], idx[1]])
    return Image.fromarray(a.astype(np.uint8), "RGBA")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", default=".")
    ap.add_argument("--width", type=int, default=2048)
    ap.add_argument("--green", default="#00FF00")
    a = ap.parse_args()

    img = build(a.width)
    bb = img.getbbox()
    if bb:
        pad = a.width // 64
        img = img.crop((max(0, bb[0] - pad), max(0, bb[1] - pad),
                        min(img.size[0], bb[2] + pad), min(img.size[1], bb[3] + pad)))
    img = bleed(img)

    os.makedirs(a.out_dir, exist_ok=True)
    p1 = os.path.join(a.out_dir, "logo_tzaar.png")
    img.save(p1)

    g = Image.new("RGBA", img.size, tuple(int(a.green[i:i + 2], 16) for i in (1, 3, 5)) + (255,))
    p2 = os.path.join(a.out_dir, "logo_tzaar_vert.png")
    Image.alpha_composite(g, img).convert("RGB").save(p2)

    al = np.array(img)[..., 3]
    print(f"{p1} : {img.size[0]}x{img.size[1]}, "
          f"{100*(al==0).mean():.1f}% transparent, {100*(al==255).mean():.1f}% opaque")
    print(f"{p2} : meme logo aplati sur {a.green}")


if __name__ == "__main__":
    main()
