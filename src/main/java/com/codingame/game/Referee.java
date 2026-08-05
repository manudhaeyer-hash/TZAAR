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
    
    private Hex parseHex(String[] parts, int offset) {
        int x = Integer.parseInt(parts[offset]);
        int y = Integer.parseInt(parts[offset+1]);
        return new Hex(x - 4, y - 4);
    }
    
    private Hex[] handleAction(Player player, String actionStr) throws Exception {
        actionStr = actionStr.trim().toUpperCase();
        if (actionStr.equals("WAIT") || actionStr.equals("PASS")) {
            if (actionPhase == 1) {
                throw new Exception("First action of the turn must be a capture.");
            }
            gameManager.addToGameSummary(player.getNicknameToken() + " passed.");
            viewer.addAction(player.getIndex(), "WAIT");
            return null;
        }
        
        String[] parts = actionStr.split("\\s+");
        if (parts[0].equals("MOVE")) {
            if (parts.length < 5) throw new Exception("Invalid MOVE command format. Expected: MOVE q1 r1 q2 r2");
            Hex from = parseHex(parts, 1);
            Hex to = parseHex(parts, 3);
            
            if (!board.isValidMove(from, to, player.getIndex())) {
                throw new Exception(String.format("Invalid move from %s to %s", from, to));
            }
            
            Stack target = board.grid.get(to);
            boolean isCapture = (target.owner != player.getIndex());
            
            if (actionPhase == 1 && !isCapture) {
                throw new Exception("First action of the turn must be a capture.");
            }
            
            board.executeMove(from, to);
            gameManager.addToGameSummary(player.getNicknameToken() + " played: " + actionStr);
            viewer.addAction(player.getIndex(), actionStr);
            return new Hex[]{from, to};
        } else {
            throw new Exception("Unknown command: " + parts[0]);
        }
    }

    @Override
    public void gameTurn(int turn) {
        Player player = gameManager.getPlayer(currentPlayerIndex);
        
        PieceType missing = board.getMissingPieceType(player.getIndex());
        if (missing != null) {
            player.message = String.format("Lost all %s pieces", missing.name());
            player.deactivate(player.message);
            player.setScore(-1);
            gameManager.addToGameSummary(player.getNicknameToken() + String.format(" lost (missing %s pieces).", missing.name()));
            gameManager.endGame();
            return;
        }
        
        if (actionPhase == 1 && !board.canMakeCapture(player.getIndex())) {
            player.message = "Cannot make any capture";
            player.deactivate(player.message);
            player.setScore(-1);
            gameManager.addToGameSummary(player.getNicknameToken() + " lost (cannot capture).");
            gameManager.endGame();
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
            player.message = "Timeout!";
            player.deactivate(player.getNicknameToken() + " timed out!");
            player.setScore(-1);
            gameManager.addToGameSummary(player.getNicknameToken() + " timed out.");
            gameManager.endGame();
            return;
        } catch (Exception e) {
            player.message = "Invalid action";
            player.deactivate(player.getNicknameToken() + ": " + e.getMessage());
            player.setScore(-1);
            gameManager.addToGameSummary(player.getNicknameToken() + " sent an invalid action.");
            gameManager.addToGameSummary(Player.getExpectedOutputFormat());
            gameManager.endGame();
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
        
        if (p0.getScore() < 0) {
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
