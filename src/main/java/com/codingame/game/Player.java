package com.codingame.game;

import com.codingame.gameengine.core.AbstractMultiplayerPlayer;

public class Player extends AbstractMultiplayerPlayer {
    public String message;

    @Override
    public int getExpectedOutputLines() {
        return 1;
    }

    public static String getExpectedOutputFormat(int actionPhase) {
        if (actionPhase == 1) {
            return "Expected output format: 'MOVE x1 y1 x2 y2'";
        }
        return "Expected output format: 'MOVE x1 y1 x2 y2' or 'WAIT'";
    }
}
