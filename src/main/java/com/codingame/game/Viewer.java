package com.codingame.game;

import com.codingame.gameengine.core.MultiplayerGameManager;
import com.codingame.gameengine.module.entities.GraphicEntityModule;
import com.codingame.gameengine.module.entities.Sprite;
import com.codingame.gameengine.module.entities.Circle;
import com.codingame.gameengine.module.entities.Curve;
import com.codingame.gameengine.module.entities.Text;
import com.codingame.gameengine.module.tooltip.TooltipModule;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Viewer {
    private GraphicEntityModule graphicEntityModule;
    private MultiplayerGameManager<Player> gameManager;
    private TooltipModule tooltipModule;
    
    private Map<Hex, List<Sprite>> hexSprites = new HashMap<>();
    private Map<Hex, Circle> hexHitboxes = new HashMap<>();
    
    private Text[] pTexts = new Text[2];
    private Text[] pScores = new Text[2];
    private Text[][] pActions = new Text[2][4];
    
    private List<String> actions0 = new ArrayList<>();
    private List<String> actions1 = new ArrayList<>();

    private static final int OFFSET_X = 1920 / 2;
    private static final int OFFSET_Y = 1080 / 2 + 20; 
    private static final double SCALE = 100.0;
    private static final int PIECE_HEIGHT = 18;

    public Viewer(GraphicEntityModule graphicEntityModule, MultiplayerGameManager<Player> gameManager, TooltipModule tooltipModule) {
        this.graphicEntityModule = graphicEntityModule;
        this.gameManager = gameManager;
        this.tooltipModule = tooltipModule;
    }

    private int getHexX(Hex hex) {
        return (int) (OFFSET_X + SCALE * 1.73205 * (hex.q + hex.r / 2.0));
    }

    private int getHexY(Hex hex) {
        return (int) (OFFSET_Y + SCALE * 0.65 * 1.5 * hex.r);
    }
    
    private String getPieceImage(int owner, PieceType type) {
        String color = owner == 0 ? "red" : "blue";
        String name = type.name().toLowerCase();
        return color + "_" + name + ".png";
    }

    public void init(Board board) {
        graphicEntityModule.createRectangle().setWidth(1920).setHeight(1080).setFillColor(0x111111);
        graphicEntityModule.createSprite()
            .setImage("board_iso.png")
            .setAnchor(0.5)
            .setX(1920 / 2)
            .setY(1080 / 2)
            .setZIndex(0);

        for (int q = -4; q <= 4; q++) {
            for (int r = -4; r <= 4; r++) {
                Hex hex = new Hex(q, r);
                if (hex.getRadius() <= 4 && hex.getRadius() > 0) {
                    int cx = getHexX(hex);
                    int cy = getHexY(hex);
                    
                    Circle hitbox = graphicEntityModule.createCircle()
                        .setRadius(30)
                        .setX(cx).setY(cy)
                        .setAlpha(0.0) 
                        .setZIndex(1000);
                    hexHitboxes.put(hex, hitbox);
                    tooltipModule.setTooltipText(hitbox, "Empty");
                }
            }
        }
        
        graphicEntityModule.createRectangle().setX(30).setY(30).setWidth(400).setHeight(140).setFillColor(0x222222).setLineColor(0xcc0000).setLineWidth(4);
        Player p0 = gameManager.getPlayer(0);
        graphicEntityModule.createSprite().setImage(p0.getAvatarToken()).setX(50).setY(50).setBaseHeight(100).setBaseWidth(100).setAnchor(0);
        pTexts[0] = graphicEntityModule.createText(p0.getNicknameToken()).setX(170).setY(60).setFontSize(40).setFillColor(0xffffff);
        pScores[0] = graphicEntityModule.createText("Pieces: 30").setX(170).setY(110).setFontSize(30).setFillColor(0xdddddd);
        
        graphicEntityModule.createRectangle().setX(1490).setY(30).setWidth(400).setHeight(140).setFillColor(0x222222).setLineColor(0x0066cc).setLineWidth(4);
        Player p1 = gameManager.getPlayer(1);
        graphicEntityModule.createSprite().setImage(p1.getAvatarToken()).setX(1870).setY(50).setBaseHeight(100).setBaseWidth(100).setAnchorX(1);
        pTexts[1] = graphicEntityModule.createText(p1.getNicknameToken()).setX(1750).setY(60).setFontSize(40).setFillColor(0xffffff).setAnchorX(1);
        pScores[1] = graphicEntityModule.createText("Pieces: 30").setX(1750).setY(110).setFontSize(30).setFillColor(0xdddddd).setAnchorX(1);

        for (int i = 0; i < 4; i++) {
            pActions[0][i] = graphicEntityModule.createText("").setX(50).setY(190 + i * 30).setFontSize(25).setFillColor(0xffffff).setZIndex(2);
            pActions[1][i] = graphicEntityModule.createText("").setX(1870).setY(190 + i * 30).setFontSize(25).setFillColor(0xffffff).setAnchorX(1).setZIndex(2);
        }

        for (Map.Entry<Hex, Stack> entry : board.grid.entrySet()) {
            Hex hex = entry.getKey();
            Stack stack = entry.getValue();
            
            List<Sprite> sprites = new ArrayList<>();
            hexSprites.put(hex, sprites);
            
            for (int i = 0; i < stack.height; i++) {
                PieceType t = (i == stack.height - 1) ? stack.type : PieceType.TOTT;
                Sprite s = graphicEntityModule.createSprite()
                    .setImage(getPieceImage(stack.owner, t))
                    .setAnchor(0.5)
                    .setX(getHexX(hex))
                    .setY(getHexY(hex) - i * PIECE_HEIGHT)
                    .setZIndex(getHexY(hex) + i);
                sprites.add(s);
            }
        }

        updateTooltips(board);
        updateScores(board);
    }
    
    public void applyMove(Hex from, Hex to, Board newBoard) {
        List<Sprite> movingSprites = hexSprites.remove(from);
        List<Sprite> targetSprites = hexSprites.get(to);
        Stack targetStack = newBoard.grid.get(to);
        
        if (movingSprites == null) return;
        
        if (targetSprites != null && !targetSprites.isEmpty()) {
            if (targetStack.height == movingSprites.size() + targetSprites.size()) {
                // Stack!
                for (int i = 0; i < movingSprites.size(); i++) {
                    Sprite s = movingSprites.get(i);
                    int newIndex = targetSprites.size() + i;
                    s.setX(getHexX(to), Curve.EASE_IN_AND_OUT);
                    s.setY(getHexY(to) - newIndex * PIECE_HEIGHT, Curve.EASE_IN_AND_OUT);
                    s.setZIndex(getHexY(to) + newIndex);
                }
                targetSprites.addAll(movingSprites);
            } else {
                // Capture!
                for (Sprite s : targetSprites) {
                    s.setAlpha(0.0);
                }
                for (int i = 0; i < movingSprites.size(); i++) {
                    Sprite s = movingSprites.get(i);
                    s.setX(getHexX(to), Curve.EASE_IN_AND_OUT);
                    s.setY(getHexY(to) - i * PIECE_HEIGHT, Curve.EASE_IN_AND_OUT);
                    s.setZIndex(getHexY(to) + i);
                }
                hexSprites.put(to, movingSprites);
            }
        } else {
            for (int i = 0; i < movingSprites.size(); i++) {
                Sprite s = movingSprites.get(i);
                s.setX(getHexX(to), Curve.EASE_IN_AND_OUT);
                s.setY(getHexY(to) - i * PIECE_HEIGHT, Curve.EASE_IN_AND_OUT);
                s.setZIndex(getHexY(to) + i);
            }
            hexSprites.put(to, movingSprites);
        }
        
        updateTooltips(newBoard);
        updateScores(newBoard);
    }
    
    public void updateTooltips(Board board) {
        for (Map.Entry<Hex, Circle> entry : hexHitboxes.entrySet()) {
            Hex hex = entry.getKey();
            Circle hitbox = entry.getValue();
            
            Stack stack = board.grid.get(hex);
            if (stack == null) {
                tooltipModule.setTooltipText(hitbox, String.format("Pos: (%d, %d)\nEmpty", hex.q + 4, hex.r + 4));
            } else {
                String ownerName = stack.owner == 0 ? "Red" : "Blue";
                String tooltipText = String.format("Pos: (%d, %d)\nOwner: %s\nType: %s\nHeight: %d", 
                    hex.q + 4, hex.r + 4,
                    ownerName,
                    stack.type.name(),
                    stack.height);
                tooltipModule.setTooltipText(hitbox, tooltipText);
            }
        }
    }
    
    public void updateScores(Board board) {
        int p0Pieces = 0;
        int p1Pieces = 0;
        for (Stack stack : board.grid.values()) {
            if (stack.owner == 0) p0Pieces++;
            else p1Pieces++;
        }
        pScores[0].setText("Stacks: " + p0Pieces);
        pScores[1].setText("Stacks: " + p1Pieces);
    }
    
    public void addAction(int playerIndex, String action) {
        List<String> list = playerIndex == 0 ? actions0 : actions1;
        list.add(action);
        if (list.size() > 4) list.remove(0);
        
        for (int i = 0; i < 4; i++) {
            if (i < list.size()) {
                pActions[playerIndex][i].setText(list.get(i));
                // Make the newest fully opaque, older ones fade out
                int age = list.size() - 1 - i;
                double alpha = Math.max(0.1, 1.0 - age * 0.22);
                pActions[playerIndex][i].setAlpha(alpha);
            } else {
                pActions[playerIndex][i].setText("");
            }
        }
    }
}
