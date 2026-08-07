#!/usr/bin/env python3
"""
Amalgame les sources en UN seul .cpp collable dans l'IDE CodinGame.

Principe : on inline recursivement les #include "..." locaux, chaque fichier
n'etant insere qu'une fois (les gardes d'inclusion sont donc respectees), et on
remonte les #include <...> systeme en tete.

  python3 tools/bundle.py                 -> submit/tzaar_bot.cpp
  python3 tools/bundle.py --out autre.cpp --strip-comments
"""
import argparse, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTRY = os.path.join(ROOT, "src", "main_cg.cpp")

# Ordre impose : les .cpp ne s'incluent pas entre eux, on les concatene apres
# les en-tetes. L'ordre reflete les dependances de construction statique.
SOURCES = [
    "src/core/geometry.cpp",
    "src/core/zobrist.cpp",
    "src/core/position.cpp",
    "src/engine/eval.cpp",
    "src/engine/tt.cpp",
    "src/engine/search.cpp",
    "src/io/protocol.cpp",
    "src/debug/trace.cpp",
    "src/main_cg.cpp",
]

LOCAL_INC = re.compile(r'^\s*#\s*include\s*"([^"]+)"\s*$')
SYS_INC = re.compile(r'^\s*#\s*include\s*<([^>]+)>\s*$')


def inline(path, seen, sysincs, out):
    real = os.path.normpath(path)
    if real in seen:
        return
    seen.add(real)
    with open(real, encoding="utf-8") as f:
        for line in f:
            m = LOCAL_INC.match(line)
            if m:
                inline(os.path.join(os.path.dirname(real), m.group(1)), seen, sysincs, out)
                continue
            m = SYS_INC.match(line)
            if m:
                sysincs.add(m.group(1))
                continue
            out.append(line.rstrip("\n"))
    out.append("")


def strip_comments(text):
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":                 # chaine : on recopie tel quel
            q = c; out.append(c); i += 1
            while i < n:
                out.append(text[i])
                if text[i] == "\\": 
                    i += 1
                    if i < n: out.append(text[i])
                elif text[i] == q:
                    i += 1; break
                i += 1
            continue
        if text.startswith("//", i):
            while i < n and text[i] != "\n": i += 1
            continue
        if text.startswith("/*", i):
            i = text.find("*/", i)
            i = n if i < 0 else i + 2
            continue
        out.append(c); i += 1
    lines = [l.rstrip() for l in "".join(out).split("\n")]
    res, blank = [], False
    for l in lines:
        if not l.strip():
            if blank: continue
            blank = True
        else:
            blank = False
        res.append(l)
    return "\n".join(res)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "submit", "tzaar_bot.cpp"))
    ap.add_argument("--strip-comments", action="store_true",
                    help="retire les commentaires (fichier plus court a coller)")
    a = ap.parse_args()

    seen, sysincs, body = set(), set(), []
    for rel in SOURCES:
        inline(os.path.join(ROOT, rel), seen, sysincs, body)

    header = [
        "// ==========================================================================",
        "// TZAAR - solver mono-fichier genere par tools/bundle.py",
        "// NE PAS EDITER : modifier les sources dans src/ puis relancer le bundler.",
        "// ==========================================================================",
        "#pragma GCC optimize(\"O3,unroll-loops\")",
        "#pragma GCC target(\"popcnt,bmi,bmi2\")",
        "",
    ]
    header += ["#include <%s>" % h for h in sorted(sysincs)]
    header.append("")

    text = "\n".join(header + body) + "\n"
    if a.strip_comments:
        text = "\n".join(header) + "\n" + strip_comments("\n".join(body)) + "\n"

    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    with open(a.out, "w", encoding="utf-8") as f:
        f.write(text)
    print("%s : %d lignes, %.1f Ko" % (a.out, text.count("\n"), len(text) / 1024))


if __name__ == "__main__":
    main()
