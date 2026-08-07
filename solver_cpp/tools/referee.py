#!/usr/bin/env python3
"""
Referee CodinGame en Python : fait jouer deux agents QUELCONQUES (executable,
'java -cp config Player', script...) via le vrai protocole stdin/stdout.

C'est le seul test qui valide la couche I/O de bout en bout ; l'arene C++, elle,
court-circuite le protocole. Sert aussi a mesurer le bot contre le Boss Java.

  python3 tools/referee.py --p0 "./tzaar" --p1 "java -cp ../config Player" -n 10

Sortie : resultat de chaque partie + bilan, et signale tout depassement de la
limite de 50 ms ou toute commande invalide.
"""
import argparse, subprocess, sys, time, shlex, os

R = 4
CELLS = [(q, r) for r in range(-R, R + 1) for q in range(max(-R, -R - r), min(R, R - r) + 1)
         if not (q == 0 and r == 0)]
IDX = {c: i for i, c in enumerate(CELLS)}
DIRS = [(1, 0), (0, 1), (-1, 1), (-1, 0), (0, -1), (1, -1)]
TOTT, TZARRA, TZAAR = 0, 1, 2
RING_DIRS = DIRS


def ring(radius):
    q, r = RING_DIRS[4][0] * radius, RING_DIRS[4][1] * radius
    out = []
    for i in range(6):
        for _ in range(radius):
            out.append((q, r))
            q += RING_DIRS[i][0]; r += RING_DIRS[i][1]
    return out


def start_board():
    """Reproduit Board.initNonRandom() du referee Java."""
    b = {}
    for i, c in enumerate(ring(1)):  b[c] = [i % 2, TOTT, 1]
    for i, c in enumerate(ring(2)):  b[c] = [(i // 2) % 2, TZAAR, 1]
    for i, c in enumerate(ring(3)):  b[c] = [(i // 3) % 2, TZARRA, 1]
    for i, c in enumerate(ring(4)):  b[c] = [(i // 4) % 2, TOTT, 1]
    return b


def first_blocker(b, c, d):
    q, r = c
    while True:
        q += d[0]; r += d[1]
        if (q, r) == (0, 0):        # le trou central bloque la ligne
            return None
        if (q, r) not in IDX:
            return None
        if (q, r) in b:
            return (q, r)


def legal_moves(b, side, phase):
    out = []
    for c, (ow, ty, h) in b.items():
        if ow != side:
            continue
        for d in DIRS:
            t = first_blocker(b, c, d)
            if t is None:
                continue
            to, tty, th = b[t]
            if to == side:
                if phase == 2: out.append((c, t))
            elif h >= th:
                out.append((c, t))
    if phase == 2: out.append(None)
    return out


def apply_move(b, mv):
    f, t = mv
    ow, ty, h = b[f]
    to, tty, th = b[t]
    del b[f]
    b[t] = [ow, ty, h + th if to == ow else h]


def missing_type(b, side):
    seen = {0: False, 1: False, 2: False}
    for (ow, ty, h) in b.values():
        if ow == side: seen[ty] = True
    for t in (0, 1, 2):
        if not seen[t]: return t
    return None


class Agent:
    def __init__(self, cmd, name):
        self.cmd, self.name = cmd, name
        self.proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                     text=True, bufsize=1)
        self.max_ms = 0.0        # hors premier tour (limite 50 ms)
        self.first_ms = 0.0      # premier tour (limite 1000 ms)
        self.total_ms = 0.0
        self.calls = 0

    def send(self, s):
        self.proc.stdin.write(s + "\n"); self.proc.stdin.flush()

    def ask(self):
        t0 = time.perf_counter()
        line = self.proc.stdout.readline()
        dt = (time.perf_counter() - t0) * 1000
        if self.calls == 0: self.first_ms = dt
        else:               self.max_ms = max(self.max_ms, dt)
        self.total_ms += dt; self.calls += 1
        if not line: raise RuntimeError(f"{self.name} a ferme sa sortie")
        return line.strip(), dt

    def close(self):
        try: self.proc.kill()
        except Exception: pass


def play(cmd0, cmd1, verbose=False, max_turns=400):
    agents = [Agent(cmd0, "P0"), Agent(cmd1, "P1")]
    for i, a in enumerate(agents): a.send(str(i))
    b = start_board()
    side, phase, turns = 0, 1, 0
    try:
        for _ in range(max_turns):
            m = missing_type(b, side)
            if m is not None:
                return 1 - side, f"J{side} a perdu tous ses pieces de type {m}", turns, agents
            moves = legal_moves(b, side, phase)
            caps = [mv for mv in moves if mv and b[mv[1]][0] != side]
            if phase == 1 and not caps:
                return 1 - side, f"J{side} ne peut pas capturer", turns, agents

            a = agents[side]
            a.send(str(phase))
            a.send(str(len(b)))
            for c, (ow, ty, h) in b.items():
                a.send(f"{c[0]+R} {c[1]+R} {ow} {ty} {h}")
            out, dt = a.ask()

            parts = out.upper().split()
            if parts and parts[0] in ("WAIT", "PASS"):
                if phase == 1:
                    return 1 - side, f"J{side}: WAIT interdit en phase 1", turns, agents
                mv = None
            elif len(parts) == 5 and parts[0] == "MOVE":
                f = (int(parts[1]) - R, int(parts[2]) - R)
                t = (int(parts[3]) - R, int(parts[4]) - R)
                mv = (f, t)
                if mv not in moves:
                    return 1 - side, f"J{side}: coup illegal {out}", turns, agents
                if phase == 1 and b[t][0] == side:
                    return 1 - side, f"J{side}: la phase 1 exige une capture", turns, agents
            else:
                return 1 - side, f"J{side}: commande invalide '{out}'", turns, agents

            if verbose: print(f"  t{turns:<3} J{side} p{phase} {out}  ({dt:.1f} ms)")
            if mv: apply_move(b, mv)

            if phase == 1:
                if turns == 0: side, phase = 1, 1
                else:          phase = 2
            else:
                phase, side = 1, 1 - side
            turns += 1
        return -1, "limite de tours", turns, agents
    finally:
        pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--p0", required=True)
    ap.add_argument("--p1", required=True)
    ap.add_argument("-n", "--games", type=int, default=4)
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--swap", action="store_true", help="alterne les couleurs")
    a = ap.parse_args()

    score = {a.p0: 0, a.p1: 0, "nulle": 0}
    worst = 0.0
    for g in range(a.games):
        c0, c1 = (a.p1, a.p0) if (a.swap and g % 2) else (a.p0, a.p1)
        w, why, turns, agents = play(c0, c1, a.verbose)
        worst = max(worst, max(x.max_ms for x in agents))
        first = max(x.first_ms for x in agents)
        for x in agents: x.close()
        name = "nulle" if w < 0 else (c0 if w == 0 else c1)
        score[name] = score.get(name, 0) + 1
        print(f"partie {g+1}: {name}  ({why}, {turns} demi-actions, "
              f"1er tour {first:.0f} ms, max ensuite {max(x.max_ms for x in agents):.1f} ms)")
    print("\n=== BILAN ===")
    for k, v in score.items(): print(f"{k}: {v}")
    print(f"temps de reponse maximum hors premier tour : {worst:.1f} ms "
          f"({'DEPASSEMENT' if worst > 50 else 'ok'} vs la limite de 50 ms)")


if __name__ == "__main__":
    main()
