package com.codingame.game;

import java.util.List;
import java.util.Map;

import com.codingame.gameengine.core.AbstractPlayer.TimeoutException;
import com.codingame.gameengine.core.AbstractReferee;
import com.codingame.gameengine.core.MultiplayerGameManager;
import com.codingame.gameengine.module.endscreen.EndScreenModule;
import com.codingame.gameengine.module.entities.GraphicEntityModule;
import com.codingame.gameengine.module.tooltip.TooltipModule;
import com.google.inject.Inject;

public class Referee extends AbstractReferee {
    @Inject private MultiplayerGameManager<Player> gameManager;
    @Inject private GraphicEntityModule graphicEntityModule;
    @Inject private EndScreenModule endScreenModule;
    @Inject private TooltipModule tooltipModule;

    static final int MAX_TURNS = 400; // 200 turns per player, each turn up to 2 actions
    
    private Board board;
    private Viewer viewer;
    private int currentPlayerIndex;
    private int actionPhase;
    private int totalTurns;

    @Override
    public void init() {
        board = new Board();
        board.initNonRandom();
        
        gameManager.setMaxTurns(MAX_TURNS);
        gameManager.setFirstTurnMaxTime(1000);
        gameManager.setTurnMaxTime(50);
        gameManager.setFrameDuration(500); // 500ms base + EASE_IN_OUT curve for smoother playback at high speeds

        viewer = new Viewer(graphicEntityModule, gameManager, tooltipModule);
        viewer.init(board);
        
        currentPlayerIndex = 0; // White starts
        actionPhase = 1;
        totalTurns = 0;
    }

    private void sendInitInputs(Player player) {
        player.sendInputLine(String.valueOf(player.getIndex()));
    }

    private void sendTurnInputs(Player player) {
        player.sendInputLine(String.valueOf(actionPhase));
        player.sendInputLine(String.valueOf(board.grid.size()));
        for (Map.Entry<Hex, Stack> entry : board.grid.entrySet()) {
            Stack stack = entry.getValue();
            player.sendInputLine(String.format("%d %d %d %d %d", 
                stack.position.q + 4, stack.position.r + 4, 
                stack.owner, stack.type.id, stack.height));
        }
    }
    
    /** Exception dont le message se suffit a lui-meme : le format attendu n'a
     *  pas a etre rappele derriere (coordonnee hors plateau, coup illegal...). */
    @SuppressWarnings("serial")
    private static class RuleException extends Exception {
        RuleException(String message) { 
            super(message);
        }
    }

    /** Exception qui, elle, justifie de rappeler le format attendu. */
    @SuppressWarnings("serial")
    private static class FormatException extends Exception {
        FormatException(String m) { super(m); }
    }

    private int parseCoordinate(String token, String name) throws FormatException {
        try {
            return Integer.parseInt(token);
        } catch (NumberFormatException e) {
            throw new FormatException(String.format("Non-integer input for %s: '%s'", name, token));
        }
    }

    /** Les coordonnees du protocole sont (x, y) dans 0..8 ; la representation
     *  interne est cubique et centree. La conversion est confinee ici, et
     *  {@link #toXY} fait le chemin inverse pour TOUT message destine au joueur :
     *  afficher des coordonnees internes n'a aucun sens de son point de vue. */
    private Hex parseHex(String[] parts, int offset) throws FormatException {
        int x = parseCoordinate(parts[offset],     offset == 1 ? "x1" : "x2");
        int y = parseCoordinate(parts[offset + 1], offset == 1 ? "y1" : "y2");
        return new Hex(x - 4, y - 4);
    }

    private static String toXY(Hex h) {
        return String.format("(%d, %d)", h.q + 4, h.r + 4);
    }
    
    private Hex[] handleAction(Player player, String actionStr) throws Exception {
        actionStr = actionStr.trim().toUpperCase();
        String[] parts = actionStr.split("\\s+");

        if (parts[0].equals("WAIT")) {
            if (parts.length != 1) {
                throw new FormatException("WAIT takes no parameter.");
            }
            if (actionPhase == 1) {
                throw new RuleException("Phase 1 requires a capture, WAIT is not allowed.");
            }
            gameManager.addToGameSummary(player.getNicknameToken() + " passed.");
            viewer.addAction(player.getIndex(), "WAIT");
            return null;
        }

        if (!parts[0].equals("MOVE")) {
            throw new FormatException("Unknown command: '" + parts[0] + "'.");
        }

        // Le nombre de parametres est verifie AVANT toute tentative d'execution :
        // sinon "MOVE 4 5 4 0 0" passait, le parametre en trop etait ignore, et
        // le joueur recevait un message de coup illegal au lieu d'un message de
        // format.
        if (parts.length != 5) {
            throw new FormatException(String.format(
                "MOVE expects exactly 4 parameters, got %d.", parts.length - 1));
        }

        Hex from = parseHex(parts, 1);
        Hex to = parseHex(parts, 3);

        if (!board.isOnBoard(from)) {
            throw new RuleException("Source " + toXY(from) + " is not a space on the board.");
        }
        if (!board.isOnBoard(to)) {
            throw new RuleException("Destination " + toXY(to) + " is not a space on the board.");
        }
        if (from.equals(to)) {
            throw new RuleException("Source and destination are the same space " + toXY(from) + ".");
        }

        Stack origin = board.grid.get(from);
        Stack target = board.grid.get(to);
        if (origin == null) {
            throw new RuleException("There is no stack on " + toXY(from) + ".");
        }
        if (origin.owner != player.getIndex()) {
            throw new RuleException("The stack on " + toXY(from) + " belongs to your opponent.");
        }
        if (target == null) {
            throw new RuleException("Destination " + toXY(to) + " is empty: a stack must land on another stack.");
        }
        if (!from.isAligned(to)) {
            throw new RuleException(String.format(
                "%s and %s are not on the same line.", toXY(from), toXY(to)));
        }
        if (!board.isPathClear(from, to)) {
            throw new RuleException(String.format(
                "The line from %s to %s is blocked (a stack, or the empty center, is in the way).",
                toXY(from), toXY(to)));
        }

        boolean isCapture = (target.owner != player.getIndex());
        if (isCapture && origin.height < target.height) {
            throw new RuleException(String.format(
                "Cannot capture: your stack on %s has height %d, the target on %s has height %d.",
                toXY(from), origin.height, toXY(to), target.height));
        }
        if (actionPhase == 1 && !isCapture) {
            throw new RuleException("Phase 1 requires a capture, not a reinforcement.");
        }

        int heightBefore = origin.height;
        board.executeMove(from, to);

        // Le resume decrit l'action, il ne repete pas la sortie brute.
        if (isCapture) {
            gameManager.addToGameSummary(String.format(
                "%s moved a %s stack (height %d) from %s to %s and captured a %s stack (height %d).",
                player.getNicknameToken(), origin.type.name().toLowerCase(), heightBefore,
                toXY(from), toXY(to), target.type.name().toLowerCase(), target.height));
        } else {
            gameManager.addToGameSummary(String.format(
                "%s moved a %s stack from %s to %s and reinforced a %s stack, making a %s stack of height %d.",
                player.getNicknameToken(), origin.type.name().toLowerCase(), heightBefore,
                toXY(from), toXY(to), target.type.name().toLowerCase(), target.height,
                origin.type.name().toLowerCase(), heightBefore + target.height));
        }
        viewer.addAction(player.getIndex(), actionStr);
        return new Hex[]{from, to};
    }

    private String describeMissing(List<PieceType> missing) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < missing.size(); i++) {
            sb.append(missing.get(i).name().toLowerCase());
            if (i < missing.size() - 2) sb.append(", ");
            else if (i == missing.size() - 2) sb.append(" and ");
        }
        return sb.toString();
    }

    private void lose(Player player, String shortMessage, String summary) {
        player.message = shortMessage;
        player.deactivate(summary);
        player.setScore(-1);
        gameManager.addToGameSummary(summary);
        gameManager.endGame();
    }

    /**
     * Verifie les conditions d'elimination pour LES DEUX joueurs, immediatement
     * apres une action.
     *
     * C'est le correctif central. Auparavant le test n'avait lieu qu'au debut du
     * tour du joueur sur le point de jouer, et seulement pour lui, ce qui
     * produisait trois anomalies :
     *   - qui capturait la derniere piece d'un type adverse en phase 1 devait
     *     tout de meme fournir une phase 2 valide pour gagner ;
     *   - qui enterrait sa propre derniere piece par un renfort n'etait declare
     *     perdant qu'a son tour suivant, laissant l'adversaire perdre avant lui ;
     *   - l'ordre des verifications, et non les regles, decidait du vainqueur.
     *
     * Verifier apres chaque action supprime les trois d'un coup : la partie
     * s'arrete a l'instant exact ou une condition devient vraie.
     *
     * @return true si la partie est terminee.
     */
    private boolean checkElimination() {
        Player p0 = gameManager.getPlayer(0);
        Player p1 = gameManager.getPlayer(1);
        List<PieceType> m0 = board.getMissingPieceTypes(0);
        List<PieceType> m1 = board.getMissingPieceTypes(1);

        // Un meme coup ne peut pas eliminer un type chez les deux joueurs : une
        // capture ne retire des pieces qu'a l'adversaire, un renfort qu'a
        // soi-meme. Le cas est donc impossible, mais on le traite plutot que de
        // dependre d'un invariant tacite.
        if (!m0.isEmpty() && !m1.isEmpty()) {
            p0.setScore(-1);
            p1.setScore(-1);
            p0.message = "Lost all " + describeMissing(m0) + " pieces";
            p1.message = "Lost all " + describeMissing(m1) + " pieces";
            gameManager.addToGameSummary("Both players lost all pieces of a type: the game is a draw.");
            gameManager.endGame();
            return true;
        }
        if (!m0.isEmpty()) {
            lose(p0, "Lost all " + describeMissing(m0) + " pieces",
                 p0.getNicknameToken() + " lost (no remaining " + describeMissing(m0) + " pieces).");
            return true;
        }
        if (!m1.isEmpty()) {
            lose(p1, "Lost all " + describeMissing(m1) + " pieces",
                 p1.getNicknameToken() + " lost (no remaining " + describeMissing(m1) + " pieces).");
            return true;
        }
        return false;
    }

    @Override
    public void gameTurn(int turn) {
        Player player = gameManager.getPlayer(currentPlayerIndex);

        // Ne pas pouvoir capturer en phase 1 depend du joueur au trait : ce test
        // reste ici, contrairement a l'elimination par type.
        if (actionPhase == 1 && !board.canMakeCapture(player.getIndex())) {
            lose(player, "No valid capture available",
                 player.getNicknameToken() + " lost (no valid capture available).");
            return;
        }

        if (totalTurns < 2 && actionPhase == 1) {
            sendInitInputs(player);
        }

        sendTurnInputs(player);
        player.execute();

        try {
            List<String> outputs = player.getOutputs();
            String output = outputs.get(0);

            Hex[] move = handleAction(player, output);
            if (move != null) {
                viewer.applyMove(move[0], move[1], board);
            }

        } catch (TimeoutException e) {
            lose(player, "Timeout!", player.getNicknameToken() + " timed out.");
            return;
        } catch (FormatException e) {
            // Seules les erreurs de FORMAT justifient de rappeler la syntaxe.
            player.message = "Invalid output";
            player.deactivate("Invalid output");
            player.setScore(-1);
            gameManager.addToGameSummary(player.getNicknameToken() + " sent an invalid output: " + e.getMessage());
            gameManager.addToGameSummary(Player.getExpectedOutputFormat(actionPhase));
            gameManager.endGame();
            return;
        } catch (RuleException e) {
            lose(player, "Invalid action",
                 player.getNicknameToken() + " played an illegal action: " + e.getMessage());
            return;
        } catch (Exception e) {
            lose(player, "Invalid action",
                 player.getNicknameToken() + " played an illegal action: " + e.getMessage());
            return;
        }

        // Elimination testee ICI, juste apres l'action, et pour les deux camps.
        if (checkElimination()) {
            return;
        }

        if (actionPhase == 1) {
            if (totalTurns == 0) {
                currentPlayerIndex = 1;
                actionPhase = 1;
            } else {
                actionPhase = 2;
            }
        } else {
            actionPhase = 1;
            currentPlayerIndex = 1 - currentPlayerIndex;
        }

        totalTurns++;
    }

    @Override
    public void onEnd() {
        int[] scores = new int[2];
        String[] texts = new String[2];
        
        Player p0 = gameManager.getPlayer(0);
        Player p1 = gameManager.getPlayer(1);
        
        // Les deux elimines : impossible en pratique (une action ne peut priver
        // d'un type que l'un des deux camps), mais si checkElimination() venait
        // a le produire, le score doit rester coherent avec le resume.
        if (p0.getScore() < 0 && p1.getScore() < 0) {
            scores[0] = 0; scores[1] = 0;
            texts[0] = p0.message != null ? p0.message : "Eliminated";
            texts[1] = p1.message != null ? p1.message : "Eliminated";
        } else if (p0.getScore() < 0) {
            scores[0] = 0; scores[1] = 1;
            texts[0] = p0.message != null ? p0.message : "Eliminated"; 
            texts[1] = "Winner";
        } else if (p1.getScore() < 0) {
            scores[0] = 1; scores[1] = 0;
            texts[0] = "Winner"; 
            texts[1] = p1.message != null ? p1.message : "Eliminated";
        } else {
            scores[0] = 0; scores[1] = 0;
            texts[0] = "Draw"; texts[1] = "Draw";
        }
        
        p0.setScore(scores[0]);
        p1.setScore(scores[1]);
        endScreenModule.setScores(scores, texts);
    }
}
