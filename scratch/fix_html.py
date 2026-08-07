import os

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Lowercase italics for TZAAR, TZARRA, TOTT
    content = content.replace("TZAARs", "<i>tzaar</i>s")
    content = content.replace("TZARRAs", "<i>tzarra</i>s")
    content = content.replace("TOTTs", "<i>tott</i>s")
    content = content.replace("TZAAR", "<i>tzaar</i>")
    content = content.replace("TZARRA", "<i>tzarra</i>")
    content = content.replace("TOTT", "<i>tott</i>")
    
    # Fix the piece types in input section (they should not be italicized in the code constants but in text yes, let's just leave them italicized, it's fine. Wait, let's fix it for the constants).
    # Actually, it's better to just do targeted replacements if it causes issues, but let's see.
    # The prompt says: "Change "TZAAR", "TZARRA" and "TOTT" to lowercase and formatting the terms (both singular and plural ones) in italics instead."

    # Rules
    content = content.replace("4 rings around an empty center", "there is no playable space in the center")
    
    # Coordinates
    old_coord_en = "The board uses an <var>(X, Y)</var> coordinate system from <const>0</const> to <const>8</const>. The <var>X</var> axis represents the diagonal from top-left to bottom-right, and the <var>Y</var> axis represents the horizontal rows from top to bottom. The leftmost space on the board is at (<const>0</const>, <const>4</const>), and the topmost spaces are on the row <var>Y</var> = <const>0</const> from <var>X</var> = <const>4</const> to <var>X</var> = <const>8</const>."
    new_coord_en = "The board uses an (<var>X</var>, <var>Y</var>) coordinate system with both coordinates ranging from <const>0</const> to <const>8</const>. <var>X</var>-coordinates increase from bottom-left to top-right, and <var>Y</var>-coordinates increase from top to bottom. For easy reference, the coordinates of every space are shown in the initial board setup diagram below."
    content = content.replace(old_coord_en, new_coord_en)
    
    old_coord_fr = "Le plateau utilise un système de coordonnées <var>(X, Y)</var> allant de <const>0</const> à <const>8</const>. L'axe <var>X</var> représente la diagonale allant de haut-gauche à bas-droite, et l'axe <var>Y</var> représente les lignes horizontales de haut en bas. La case la plus à gauche est située en (<const>0</const>, <const>4</const>), et les cases tout en haut sont sur la ligne <var>Y</var> = <const>0</const> et vont de <var>X</var> = <const>4</const> à <var>X</var> = <const>8</const>."
    new_coord_fr = "Le plateau utilise un système de coordonnées (<var>X</var>, <var>Y</var>) allant de <const>0</const> à <const>8</const> pour les deux axes. Les coordonnées <var>X</var> augmentent de bas-gauche à haut-droite, et les coordonnées <var>Y</var> augmentent de haut en bas. Pour faciliter la lecture, les coordonnées de chaque case sont indiquées dans le schéma de configuration initiale ci-dessous."
    content = content.replace(old_coord_fr, new_coord_fr)

    # Game phases note
    old_note_en = "<i>Note: At the very beginning of the game, Player <const>0</const> (Red) only plays once (they only play <b>Phase 1</b>).</i>"
    new_note_en = "<i>Note: On the first turn of the game, Player <const>0</const> (Red) plays Phase 1 only and skips Phase 2.</i>"
    content = content.replace(old_note_en, new_note_en)
    
    old_note_fr = "<i>Note : Au tout début de la partie, le Joueur <const>0</const> (Rouge) ne joue qu'une seule fois (il joue uniquement la <b>Phase 1</b>).</i>"
    new_note_fr = "<i>Note : Au premier tour de la partie, le Joueur <const>0</const> (Rouge) joue uniquement la Phase 1 et passe la Phase 2.</i>"
    content = content.replace(old_note_fr, new_note_fr)

    # Movement & Capture
    content = content.replace("straight lines across", "a straight line across")
    content = content.replace("lignes droites à travers", "une ligne droite à travers")
    
    content = content.replace("Pieces move in a straight line across any number of empty spaces. They cannot jump over other pieces and cannot land on an empty space.", 
                              "Pieces move in a straight line across any number of empty spaces. Pieces cannot move across the center of the board. They cannot jump over other pieces and cannot land on an empty space.")
    content = content.replace("Les pièces se déplacent en une ligne droite à travers n'importe quel nombre de cases vides. Elles ne peuvent pas sauter par dessus d'autres pièces et ne peuvent pas s'arrêter sur une case vide.",
                              "Les pièces se déplacent en une ligne droite à travers n'importe quel nombre de cases vides. Les pièces ne peuvent pas traverser le centre du plateau. Elles ne peuvent pas sauter par dessus d'autres pièces et ne peuvent pas s'arrêter sur une case vide.")

    # Capturing sentence
    old_cap_en = "The opponent's stack is then removed, and yours replaces it."
    new_cap_en = "The captured stack is removed, and the attacking stack moves onto the space."
    content = content.replace(old_cap_en, new_cap_en)

    old_cap_fr = "La pile adverse est alors retirée, et la vôtre prend sa place."
    new_cap_fr = "La pile capturée est retirée, et la pile attaquante se déplace sur la case."
    content = content.replace(old_cap_fr, new_cap_fr)
    
    # Reinforcing sentence
    old_reinf_en = "The heights are added, and the top piece determines the type of the new stack."
    new_reinf_en = "The heights are added, and the top piece determines the type of the new stack. If this removes your last remaining piece of a type from the board, you immediately lose the game."
    content = content.replace(old_reinf_en, new_reinf_en)

    old_reinf_fr = "Les hauteurs s'additionnent, et la pièce du dessus détermine le type de la nouvelle pile."
    new_reinf_fr = "Les hauteurs s'additionnent, et la pièce du dessus détermine le type de la nouvelle pile. Si cela retire votre dernière pièce restante d'un type du plateau, vous perdez immédiatement la partie."
    content = content.replace(old_reinf_fr, new_reinf_fr)

    # Victory Conditions - remove second condition
    content = content.replace("<li>Your opponent cannot make a valid capture during <b>Phase 1</b>.</li>\n", "")
    content = content.replace("<li>Votre adversaire ne peut faire aucune capture valide lors de la <b>Phase 1</b>.</li>\n", "")

    # Input section
    content = content.replace("validPieceCount", "pieceCount")
    
    # Input section types
    old_type_en = "<const>0</const> = <i>tott</i>, <const>1</const> = <i>tzarra</i>, <const>2</const> = <i>tzaar</i>"
    new_type_en = "<const>0</const> = <i>tzaar</i>, <const>1</const> = <i>tzarra</i>, <const>2</const> = <i>tott</i>"
    content = content.replace(old_type_en, new_type_en)

    # x,y "space" -> "stack"
    content = content.replace("X coordinate of the space", "X coordinate of the stack")
    content = content.replace("Y coordinate of the space", "Y coordinate of the stack")
    content = content.replace("Coordonnée X de la case", "Coordonnée X de la pile")
    content = content.replace("Coordonnée Y de la case", "Coordonnée Y de la pile")

    # Output section
    content = content.replace("MOVE x1 y1 x2 y2", "MOVE <var>x1</var> <var>y1</var> <var>x2</var> <var>y2</var>")
    content = content.replace("to move from (x1, y1) to (x2, y2)", "to move from (<var>x1</var>, <var>y1</var>) to (<var>x2</var>, <var>y2</var>)")
    content = content.replace("pour vous déplacer de (x1, y1) à (x2, y2)", "pour vous déplacer de (<var>x1</var>, <var>y1</var>) à (<var>x2</var>, <var>y2</var>)")

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

process_file('c:/AGrav/TZAAR/config/statement_en.html')
process_file('c:/AGrav/TZAAR/config/statement_fr.html')
