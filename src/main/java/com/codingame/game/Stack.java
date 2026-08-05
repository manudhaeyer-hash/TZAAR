package com.codingame.game;

public class Stack {
    public int owner;
    public PieceType type;
    public int height;
    public Hex position;

    public Stack(int owner, PieceType type, int height, Hex position) {
        this.owner = owner;
        this.type = type;
        this.height = height;
        this.position = position;
    }

    public Stack copy() {
        return new Stack(owner, type, height, position);
    }
}
