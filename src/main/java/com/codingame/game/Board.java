package com.codingame.game;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Board {
    public Map<Hex, Stack> grid = new HashMap<>();

    private static final Hex[] hex_directions = new Hex[]{
        new Hex(1, 0, -1), new Hex(0, 1, -1), new Hex(-1, 1, 0),
        new Hex(-1, 0, 1), new Hex(0, -1, 1), new Hex(1, -1, 0)
    };

    public List<Hex> getRing(int radius) {
        List<Hex> results = new ArrayList<>();
        if (radius == 0) {
            results.add(new Hex(0, 0, 0));
            return results;
        }
        
        Hex hex = hex_directions[4].scale(radius);
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < radius; j++) {
                results.add(hex);
                hex = hex.add(hex_directions[i]);
            }
        }
        return results;
    }

    public void initNonRandom() {
        grid.clear();
        
        List<Hex> ring1 = getRing(1);
        for (int i = 0; i < ring1.size(); i++) {
            grid.put(ring1.get(i), new Stack(i % 2, PieceType.TOTT, 1, ring1.get(i)));
        }
        
        List<Hex> ring2 = getRing(2);
        for (int i = 0; i < ring2.size(); i++) {
            int owner = (i / 2) % 2;
            grid.put(ring2.get(i), new Stack(owner, PieceType.TZAAR, 1, ring2.get(i)));
        }
        
        List<Hex> ring3 = getRing(3);
        for (int i = 0; i < ring3.size(); i++) {
            int owner = (i / 3) % 2;
            grid.put(ring3.get(i), new Stack(owner, PieceType.TZARRA, 1, ring3.get(i)));
        }
        
        List<Hex> ring4 = getRing(4);
        for (int i = 0; i < ring4.size(); i++) {
            int owner = (i / 4) % 2;
            grid.put(ring4.get(i), new Stack(owner, PieceType.TOTT, 1, ring4.get(i)));
        }
    }
    
    public boolean isPathClear(Hex a, Hex b) {
        if (!a.isAligned(b)) return false;
        
        int dist = a.distance(b);
        if (dist == 0) return true;
        
        int dq = (b.q - a.q) / dist;
        int dr = (b.r - a.r) / dist;
        
        Hex current = new Hex(a.q + dq, a.r + dr);
        while (!current.equals(b)) {
            if (current.q == 0 && current.r == 0) return false; 
            if (grid.containsKey(current)) return false;
            current = new Hex(current.q + dq, current.r + dr);
        }
        return true;
    }

    public boolean isValidMove(Hex from, Hex to, int player) {
        Stack origin = grid.get(from);
        Stack target = grid.get(to);
        
        if (origin == null || target == null) return false;
        if (origin.owner != player) return false;
        if (from.equals(to)) return false;
        if (to.q == 0 && to.r == 0) return false;
        
        if (!isPathClear(from, to)) return false;
        
        if (target.owner != player) {
            if (origin.height < target.height) return false;
        }
        
        return true;
    }
    
    public void executeMove(Hex from, Hex to) {
        Stack origin = grid.get(from);
        Stack target = grid.get(to);
        
        grid.remove(from);
        origin.position = to;
        
        if (target.owner == origin.owner) {
            origin.height += target.height;
        }
        
        grid.put(to, origin);
    }
    
    public List<PieceType> getMissingPieceTypes(int player) {
        boolean hasTott = false;
        boolean hasTzarra = false;
        boolean hasTzaar = false;
        
        for (Stack stack : grid.values()) {
            if (stack.owner == player) {
                if (stack.type == PieceType.TOTT) hasTott = true;
                if (stack.type == PieceType.TZARRA) hasTzarra = true;
                if (stack.type == PieceType.TZAAR) hasTzaar = true;
            }
        }
        
        List<PieceType> missing = new ArrayList<>();
        if (!hasTzaar) missing.add(PieceType.TZAAR);
        if (!hasTzarra) missing.add(PieceType.TZARRA);
        if (!hasTott) missing.add(PieceType.TOTT);
        
        return missing;
    }
    
    public boolean canMakeCapture(int player) {
        List<Stack> myStacks = new ArrayList<>();
        List<Stack> oppStacks = new ArrayList<>();
        
        for (Stack stack : grid.values()) {
            if (stack.owner == player) {
                myStacks.add(stack);
            } else {
                oppStacks.add(stack);
            }
        }
        
        for (Stack me : myStacks) {
            for (Stack opp : oppStacks) {
                if (me.height >= opp.height && isPathClear(me.position, opp.position)) {
                    return true;
                }
            }
        }
        return false;
    }
}
