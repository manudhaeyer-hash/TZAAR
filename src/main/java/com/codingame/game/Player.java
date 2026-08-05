package com.codingame.game;

import com.codingame.gameengine.core.AbstractMultiplayerPlayer;

public class Player extends AbstractMultiplayerPlayer {
    public String message;

    @Override
    public int getExpectedOutputLines() {
        return 1;
    }

    public static String getExpectedOutputFormat() {
        return "Expected output format: 'MOVE q1 r1 q2 r2' or 'WAIT'";
    }
}
