package com.codingame.game;

import java.util.Objects;

public class Hex {
    public final int q;
    public final int r;
    public final int s;

    public Hex(int q, int r) {
        this.q = q;
        this.r = r;
        this.s = -q - r;
    }

    public Hex(int q, int r, int s) {
        this.q = q;
        this.r = r;
        this.s = s;
    }

    public int distance(Hex b) {
        return (Math.abs(q - b.q) + Math.abs(r - b.r) + Math.abs(s - b.s)) / 2;
    }

    public boolean isAligned(Hex b) {
        return this.q == b.q || this.r == b.r || this.s == b.s;
    }
    
    public boolean isBetween(Hex a, Hex b) {
        if (!a.isAligned(b)) return false;
        if (!this.isAligned(a) || !this.isAligned(b)) return false;
        return (a.distance(this) + this.distance(b) == a.distance(b));
    }
    
    public int getRadius() {
        return (Math.abs(q) + Math.abs(r) + Math.abs(s)) / 2;
    }

    public Hex add(Hex b) {
        return new Hex(this.q + b.q, this.r + b.r, this.s + b.s);
    }
    
    public Hex scale(int k) {
        return new Hex(this.q * k, this.r * k, this.s * k);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Hex hex = (Hex) o;
        return q == hex.q && r == hex.r && s == hex.s;
    }

    @Override
    public int hashCode() {
        return Objects.hash(q, r, s);
    }
    
    @Override
    public String toString() {
        return "(" + q + "," + r + ")";
    }
}
