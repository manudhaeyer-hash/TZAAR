import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class GenSVG {
    
    static class Hex {
        int q, r, s;
        Hex(int q, int r, int s) { this.q = q; this.r = r; this.s = s; }
    }

    static final Hex[] dirs = new Hex[]{
        new Hex(1, 0, -1), new Hex(0, 1, -1), new Hex(-1, 1, 0),
        new Hex(-1, 0, 1), new Hex(0, -1, 1), new Hex(1, -1, 0)
    };

    static Hex add(Hex h1, Hex h2) {
        return new Hex(h1.q+h2.q, h1.r+h2.r, h1.s+h2.s);
    }
    static Hex scale(Hex h, int k) {
        return new Hex(h.q*k, h.r*k, h.s*k);
    }

    static List<Hex> getRing(int radius) {
        List<Hex> results = new ArrayList<>();
        if (radius == 0) return results;
        Hex hex = scale(dirs[4], radius);
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < radius; j++) {
                results.add(hex);
                hex = add(hex, dirs[i]);
            }
        }
        return results;
    }

    static final int SIZE = 40;
    static final int WIDTH = 800;
    static final int HEIGHT = 700;

    static double[] getXY(int q, int r) {
        double x = WIDTH/2.0 + SIZE * Math.sqrt(3) * (q + r/2.0);
        double y = HEIGHT/2.0 + SIZE * 1.5 * r;
        return new double[]{x, y};
    }

    static String hexPoly(double x, double y) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 6; i++) {
            double angleDeg = 60 * i - 30;
            double angleRad = Math.PI / 180 * angleDeg;
            sb.append(String.format(java.util.Locale.US, "%.2f,%.2f ", x + SIZE * Math.cos(angleRad), y + SIZE * Math.sin(angleRad)));
        }
        return sb.toString();
    }

    public static void main(String[] args) throws Exception {
        Map<String, String> gridType = new HashMap<>();
        Map<String, Integer> gridOwner = new HashMap<>();

        List<Hex> r1 = getRing(1);
        for (int i=0; i<r1.size(); i++) {
            String k = r1.get(i).q+","+r1.get(i).r;
            gridType.put(k, "TOTT"); gridOwner.put(k, i%2);
        }
        List<Hex> r2 = getRing(2);
        for (int i=0; i<r2.size(); i++) {
            String k = r2.get(i).q+","+r2.get(i).r;
            gridType.put(k, "TZAAR"); gridOwner.put(k, (i/2)%2);
        }
        List<Hex> r3 = getRing(3);
        for (int i=0; i<r3.size(); i++) {
            String k = r3.get(i).q+","+r3.get(i).r;
            gridType.put(k, "TZARRA"); gridOwner.put(k, (i/3)%2);
        }
        List<Hex> r4 = getRing(4);
        for (int i=0; i<r4.size(); i++) {
            String k = r4.get(i).q+","+r4.get(i).r;
            gridType.put(k, "TOTT"); gridOwner.put(k, (i/4)%2);
        }

        List<Hex> all = new ArrayList<>();
        for(int r=1; r<=4; r++) all.addAll(getRing(r));

        PrintWriter out = new PrintWriter("c:/AGrav/TZAAR/config/board_init.svg");
        out.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        out.println("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "+WIDTH+" "+HEIGHT+"\" style=\"max-width: 100%; height: auto;\">");

        for (Hex h : all) {
            double[] pos = getXY(h.q, h.r);
            out.println("<polygon points=\""+hexPoly(pos[0], pos[1])+"\" fill=\"#333333\" stroke=\"#555555\" stroke-width=\"2\" />");
        }

        for (Hex h : all) {
            double[] pos = getXY(h.q, h.r);
            out.println(String.format(java.util.Locale.US, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"Arial\" font-size=\"14\" fill=\"#bbbbbb\" text-anchor=\"middle\">%d,%d</text>", 
                pos[0], pos[1] - SIZE*0.65, h.q+4, h.r+4));
        }

        for (Hex h : all) {
            double[] pos = getXY(h.q, h.r);
            String k = h.q+","+h.r;
            if(!gridType.containsKey(k)) continue;
            int owner = gridOwner.get(k);
            String type = gridType.get(k);
            String color = owner == 0 ? "#cc0000" : "#0066cc";
            out.println(String.format(java.util.Locale.US, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\" stroke=\"#ffffff\" stroke-width=\"2\" />", pos[0], pos[1], SIZE*0.5, color));
            if (type.equals("TZARRA")) {
                out.println(String.format(java.util.Locale.US, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"#ffffff\" />", pos[0], pos[1], SIZE*0.15));
            } else if (type.equals("TZAAR")) {
                out.println(String.format(java.util.Locale.US, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"3\" />", pos[0], pos[1], SIZE*0.25));
            }
        }

        out.println("</svg>");
        out.close();
    }
}
