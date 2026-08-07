import math

SIZE = 45
WIDTH = 1000
HEIGHT = 900

def get_xy(q, r):
    x = WIDTH/2 + SIZE * math.sqrt(3) * (q + r/2.0)
    y = HEIGHT/2 + SIZE * 1.5 * r
    return x, y

def hex_poly(x, y):
    pts = []
    for i in range(6):
        angle_deg = 60 * i - 30
        angle_rad = math.pi / 180 * angle_deg
        pts.append(f"{x + SIZE * math.cos(angle_rad):.2f},{y + SIZE * math.sin(angle_rad):.2f}")
    return " ".join(pts)

directions = [
    (1, 0, -1), (0, 1, -1), (-1, 1, 0),
    (-1, 0, 1), (0, -1, 1), (1, -1, 0)
]

def add(h1, h2): return (h1[0]+h2[0], h1[1]+h2[1], h1[2]+h2[2])
def scale(h, k): return (h[0]*k, h[1]*k, h[2]*k)

def get_ring(radius):
    if radius == 0: return [(0,0,0)]
    results = []
    hex_pos = scale(directions[4], radius)
    for i in range(6):
        for j in range(radius):
            results.append(hex_pos)
            hex_pos = add(hex_pos, directions[i])
    return results

grid = {}
for i, h in enumerate(get_ring(1)): grid[h] = (i%2, 'TOTT')
for i, h in enumerate(get_ring(2)): grid[h] = ((i//2)%2, 'TZAAR')
for i, h in enumerate(get_ring(3)): grid[h] = ((i//3)%2, 'TZARRA')
for i, h in enumerate(get_ring(4)): grid[h] = ((i//4)%2, 'TOTT')

# all valid hexes up to radius 4 (without center)
all_hexes = []
for r in range(1, 5):
    all_hexes.extend(get_ring(r))

out = []
out.append('<?xml version="1.0" encoding="UTF-8"?>')
out.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">')
out.append('<rect width="100%" height="100%" fill="#1e1e1e" />')

# Draw hexes
for h in all_hexes:
    q, r_ax, s = h
    x, y = get_xy(q, r_ax)
    out.append(f'<polygon points="{hex_poly(x, y)}" fill="#333333" stroke="#555555" stroke-width="2" />')

# Draw coordinates
for h in all_hexes:
    q, r_ax, s = h
    x, y = get_xy(q, r_ax)
    user_x = q + 4
    user_y = r_ax + 4
    out.append(f'<text x="{x}" y="{y - SIZE*0.65:.2f}" font-family="Arial" font-size="12" fill="#aaaaaa" text-anchor="middle">{user_x}, {user_y}</text>')

# Draw pieces
for h, (owner, ptype) in grid.items():
    q, r_ax, s = h
    x, y = get_xy(q, r_ax)
    color = "#cc0000" if owner == 0 else "#0066cc"
    
    # Outer piece circle
    out.append(f'<circle cx="{x}" cy="{y}" r="{SIZE*0.55:.2f}" fill="{color}" stroke="#ffffff" stroke-width="2" />')
    
    # Piece type icon
    if ptype == 'TZAAR':
        out.append(f'<circle cx="{x}" cy="{y}" r="{SIZE*0.15:.2f}" fill="#ffffff" />')
    elif ptype == 'TZARRA':
        out.append(f'<circle cx="{x}" cy="{y}" r="{SIZE*0.25:.2f}" fill="none" stroke="#ffffff" stroke-width="3" />')

out.append('</svg>')

with open('c:/AGrav/TZAAR/config/board_init.svg', 'w') as f:
    f.write("\n".join(out))
