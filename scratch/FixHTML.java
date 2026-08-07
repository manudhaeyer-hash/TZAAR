import java.nio.file.Files;
import java.nio.file.Paths;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class FixHTML {
    public static void main(String[] args) throws IOException {
        processFile("c:/AGrav/TZAAR/config/statement_en.html");
        processFile("c:/AGrav/TZAAR/config/statement_fr.html");
    }

    private static void processFile(String filepath) throws IOException {
        String content = new String(Files.readAllBytes(Paths.get(filepath)), StandardCharsets.UTF_8);

        content = content.replace("TZAARs", "<i>tzaar</i>s");
        content = content.replace("TZARRAs", "<i>tzarra</i>s");
        content = content.replace("TOTTs", "<i>tott</i>s");
        content = content.replace("TZAAR", "<i>tzaar</i>");
        content = content.replace("TZARRA", "<i>tzarra</i>");
        content = content.replace("TOTT", "<i>tott</i>");

        content = content.replace("4 rings around an empty center", "there is no playable space in the center");

        String oldCoordEn = "The board uses an <var>(X, Y)</var> coordinate system from <const>0</const> to <const>8</const>. The <var>X</var> axis represents the diagonal from top-left to bottom-right, and the <var>Y</var> axis represents the horizontal rows from top to bottom. The leftmost space on the board is at (<const>0</const>, <const>4</const>), and the topmost spaces are on the row <var>Y</var> = <const>0</const> from <var>X</var> = <const>4</const> to <var>X</var> = <const>8</const>.";
        String newCoordEn = "The board uses an (<var>X</var>, <var>Y</var>) coordinate system with both coordinates ranging from <const>0</const> to <const>8</const>. <var>X</var>-coordinates increase from bottom-left to top-right, and <var>Y</var>-coordinates increase from top to bottom. For easy reference, the coordinates of every space are shown in the initial board setup diagram below.";
        content = content.replace(oldCoordEn, newCoordEn);

        String oldCoordFr = "Le plateau utilise un système de coordonnées <var>(X, Y)</var> allant de <const>0</const> à <const>8</const>. L'axe <var>X</var> représente la diagonale allant de haut-gauche à bas-droite, et l'axe <var>Y</var> représente les lignes horizontales de haut en bas. La case la plus à gauche est située en (<const>0</const>, <const>4</const>), et les cases tout en haut sont sur la ligne <var>Y</var> = <const>0</const> et vont de <var>X</var> = <const>4</const> à <var>X</var> = <const>8</const>.";
        String newCoordFr = "Le plateau utilise un système de coordonnées (<var>X</var>, <var>Y</var>) avec les deux coordonnées allant de <const>0</const> à <const>8</const>. Les coordonnées <var>X</var> augmentent de bas-gauche à haut-droite, et les coordonnées <var>Y</var> augmentent de haut en bas. Pour faciliter la lecture, les coordonnées de chaque case sont indiquées dans le schéma de configuration initiale ci-dessous.";
        content = content.replace(oldCoordFr, newCoordFr);

        String oldNoteEn = "<i>Note: At the very beginning of the game, Player <const>0</const> (Red) only plays once (they only play <b>Phase 1</b>).</i>";
        String newNoteEn = "<i>Note: On the first turn of the game, Player <const>0</const> (Red) plays Phase 1 only and skips Phase 2.</i>";
        content = content.replace(oldNoteEn, newNoteEn);

        String oldNoteFr = "<i>Note : Au tout début de la partie, le Joueur <const>0</const> (Rouge) ne joue qu'une seule fois (il joue uniquement la <b>Phase 1</b>).</i>";
        String newNoteFr = "<i>Note : Au premier tour de la partie, le Joueur <const>0</const> (Rouge) joue uniquement la Phase 1 et passe la Phase 2.</i>";
        content = content.replace(oldNoteFr, newNoteFr);

        content = content.replace("straight lines across", "a straight line across");
        content = content.replace("lignes droites à travers", "une ligne droite à travers");

        content = content.replace("Pieces move in a straight line across any number of empty spaces. They cannot jump over other pieces and cannot land on an empty space.", 
                                  "Pieces move in a straight line across any number of empty spaces. Pieces cannot move across the center of the board. They cannot jump over other pieces and cannot land on an empty space.");
        content = content.replace("Les pièces se déplacent en une ligne droite à travers n'importe quel nombre de cases vides. Elles ne peuvent pas sauter par dessus d'autres pièces et ne peuvent pas s'arrêter sur une case vide.",
                                  "Les pièces se déplacent en une ligne droite à travers n'importe quel nombre de cases vides. Les pièces ne peuvent pas traverser le centre du plateau. Elles ne peuvent pas sauter par dessus d'autres pièces et ne peuvent pas s'arrêter sur une case vide.");

        String oldCapEn = "The opponent's stack is then removed, and yours replaces it.";
        String newCapEn = "The captured stack is removed, and the attacking stack moves onto the space.";
        content = content.replace(oldCapEn, newCapEn);

        String oldCapFr = "La pile adverse est alors retirée, et la vôtre prend sa place.";
        String newCapFr = "La pile capturée est retirée, et la pile attaquante se déplace sur la case.";
        content = content.replace(oldCapFr, newCapFr);

        String oldReinfEn = "The heights are added, and the top piece determines the type of the new stack.";
        String newReinfEn = "The heights are added, and the top piece determines the type of the new stack. If this removes your last remaining piece of a type from the board, you immediately lose the game.";
        content = content.replace(oldReinfEn, newReinfEn);

        String oldReinfFr = "Les hauteurs s'additionnent, et la pièce du dessus détermine le type de la nouvelle pile.";
        String newReinfFr = "Les hauteurs s'additionnent, et la pièce du dessus détermine le type de la nouvelle pile. Si cela retire votre dernière pièce restante d'un type du plateau, vous perdez immédiatement la partie.";
        content = content.replace(oldReinfFr, newReinfFr);

        content = content.replace("<li>Your opponent cannot make a valid capture during <b>Phase 1</b>.</li>\n", "");
        content = content.replace("<li>Votre adversaire ne peut faire aucune capture valide lors de la <b>Phase 1</b>.</li>\n", "");
        content = content.replace("<li>Your opponent cannot make a valid capture during <b>Phase 1</b>.</li>\r\n", "");
        content = content.replace("<li>Votre adversaire ne peut faire aucune capture valide lors de la <b>Phase 1</b>.</li>\r\n", "");

        content = content.replace("validPieceCount", "pieceCount");

        String oldTypeEn = "<const>0</const> = <i><i>tott</i></i>, <const>1</const> = <i><i>tzarra</i></i>, <const>2</const> = <i><i>tzaar</i></i>";
        String newTypeEn = "<const>0</const> = <i>tzaar</i>, <const>1</const> = <i>tzarra</i>, <const>2</const> = <i>tott</i>";
        content = content.replace(oldTypeEn, newTypeEn);

        String oldTypeFr = "<const>0</const> = <i><i>tott</i></i>, <const>1</const> = <i><i>tzarra</i></i>, <const>2</const> = <i><i>tzaar</i></i>";
        String newTypeFr = "<const>0</const> = <i>tzaar</i>, <const>1</const> = <i>tzarra</i>, <const>2</const> = <i>tott</i>";
        content = content.replace(oldTypeFr, newTypeFr);

        content = content.replace("X coordinate of the space", "X coordinate of the stack");
        content = content.replace("Y coordinate of the space", "Y coordinate of the stack");
        content = content.replace("Coordonnée X de la case", "Coordonnée X de la pile");
        content = content.replace("Coordonnée Y de la case", "Coordonnée Y de la pile");

        content = content.replace("MOVE x1 y1 x2 y2", "MOVE <var>x1</var> <var>y1</var> <var>x2</var> <var>y2</var>");
        content = content.replace("to move from (x1, y1) to (x2, y2)", "to move from (<var>x1</var>, <var>y1</var>) to (<var>x2</var>, <var>y2</var>)");
        content = content.replace("pour vous déplacer de (x1, y1) à (x2, y2)", "pour vous déplacer de (<var>x1</var>, <var>y1</var>) à (<var>x2</var>, <var>y2</var>)");

        Files.write(Paths.get(filepath), content.getBytes(StandardCharsets.UTF_8));
    }
}
