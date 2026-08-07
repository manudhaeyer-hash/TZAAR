#!/usr/bin/env python3
"""
Detoure une image posee sur un fond de couleur unie (vert fluo, magenta...).

A utiliser si VOUS generez le logo avec un outil d'image en demandant un fond
uni : c'est la moitie du travail que je ne peux pas faire, celle-ci je la fais.

Un fond uni est plus simple qu'un damier, mais deux pieges demeurent :

  1. Le DEBORDEMENT DE TEINTE. Sur les bords antialiases, le pixel est un
     melange du logo et du vert. Un seuillage binaire laisse une frange verte.
     On resout P = a*F + (1-a)*V : la distance a la couleur de fond donne
     alpha, puis on retire la part de vert de la couleur.

  2. Le NOIR SOUS L'ALPHA. Mettre a zero le RGB des pixels transparents parait
     propre, mais tout moteur de rendu interpole RGB et alpha separement : ce
     noir remonte alors dans les contours. On recopie donc la couleur du bord
     dans toute la zone transparente.

  python3 chroma_key.py entree.png -o sortie.png --trim        # fond detecte
  python3 chroma_key.py entree.png -o sortie.png --key 00FF00 --tol 60
"""
import argparse
import numpy as np
from PIL import Image
from scipy import ndimage


def detect_key(rgb):
    """Couleur de fond = la teinte dominante du cadre de bord de l'image."""
    H, W = rgb.shape[:2]
    b = max(2, min(H, W) // 40)
    bord = np.concatenate([rgb[:b].reshape(-1, 3), rgb[-b:].reshape(-1, 3),
                           rgb[:, :b].reshape(-1, 3), rgb[:, -b:].reshape(-1, 3)])
    q = (bord // 8).astype(int)
    cle = q[:, 0] * 1024 + q[:, 1] * 32 + q[:, 2]
    codes, counts = np.unique(cle, return_counts=True)
    return bord[cle == codes[np.argmax(counts)]].mean(0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("--key", help="couleur de fond en hexa, ex 00FF00 (sinon detectee)")
    ap.add_argument("--tol", type=float, default=55.0,
                    help="distance en deca de laquelle un pixel est du fond pur")
    ap.add_argument("--soft", type=float, default=45.0,
                    help="largeur de la zone de transition (alpha partiel)")
    ap.add_argument("--despill", type=float, default=1.0,
                    help="force de la correction de debordement de teinte, 0 a 1")
    ap.add_argument("--trim", action="store_true", help="recadrer sur le contenu")
    a = ap.parse_args()

    im = Image.open(a.input).convert("RGB")
    rgb = np.array(im).astype(float)

    key = (np.array([int(a.key[i:i + 2], 16) for i in (0, 2, 4)], float)
           if a.key else detect_key(rgb))
    print(f"couleur de fond : R{key[0]:.0f} V{key[1]:.0f} B{key[2]:.0f}")

    # --- alpha : distance a la couleur de fond -------------------------------
    d = np.linalg.norm(rgb - key[None, None, :], axis=-1)
    alpha = np.clip((d - a.tol) / max(a.soft, 1e-6), 0, 1)

    # Un pixel isole ne fait pas un fond : on ne garde que la partie reliee au
    # bord de l'image, sinon les zones du logo qui frolent la teinte du fond
    # seraient percees de trous.
    fond = alpha < 0.5
    lab, n = ndimage.label(fond, structure=np.ones((3, 3)))
    bords = set(lab[0].tolist()) | set(lab[-1].tolist()) | \
            set(lab[:, 0].tolist()) | set(lab[:, -1].tolist())
    bords.discard(0)
    fond_reel = np.isin(lab, list(bords))
    alpha = np.where(fond & ~fond_reel, 1.0, alpha)

    # --- despill : retirer la part de fond melangee au bord ------------------
    if a.despill > 0:
        with np.errstate(invalid="ignore", divide="ignore"):
            F = (rgb - (1 - alpha[..., None]) * key[None, None, :]) / np.maximum(alpha[..., None], 1e-3)
        band = (alpha > 0.02) & (alpha < 0.995)
        rgb = np.where(band[..., None],
                       rgb * (1 - a.despill) + np.clip(F, 0, 255) * a.despill, rgb)

    # --- debordement de couleur sous l'alpha ---------------------------------
    op = alpha > 0
    if op.any() and (~op).any():
        idx = ndimage.distance_transform_edt(~op, return_distances=False, return_indices=True)
        rgb = np.where(op[..., None], rgb, rgb[idx[0], idx[1]])

    out = np.dstack([np.clip(rgb, 0, 255), alpha * 255]).astype(np.uint8)
    img = Image.fromarray(out, "RGBA")
    if a.trim:
        bb = img.getbbox()
        if bb:
            img = img.crop(bb)
    img.save(a.output)
    print(f"ecrit {a.output} : {img.size[0]}x{img.size[1]}, "
          f"{100*(alpha==0).mean():.1f}% transparent, {100*(alpha==1).mean():.1f}% opaque")


if __name__ == "__main__":
    main()
