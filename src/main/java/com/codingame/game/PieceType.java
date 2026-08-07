package com.codingame.game;

public enum PieceType {
    TZAAR(0),
    TZARRA(1),
    TOTT(2);

    public final int id;

    PieceType(int id) {
        this.id = id;
    }

    public static PieceType fromId(int id) {
        for (PieceType type : values()) {
            if (type.id == id) return type;
        }
        return TOTT;
    }
}
