import java.nio.file.Files;
import java.nio.file.Paths;
import java.io.IOException;

public class InjectSVG {
    public static void main(String[] args) throws IOException {
        String svg = new String(Files.readAllBytes(Paths.get("c:/AGrav/TZAAR/config/board_init.svg")));
        
        String enMarker = "from <var>X = 4</var> to <var>X = 8</var>.<br>";
        String enStr = new String(Files.readAllBytes(Paths.get("c:/AGrav/TZAAR/config/statement_en.html")));
        String enInsert = enMarker + "\n        <details><summary>Show initial board setup</summary>\n        <div style=\"text-align: center;\">\n" + svg + "\n        </div>\n        </details>";
        if (!enStr.contains("<details><summary>Show initial board setup</summary>")) {
            enStr = enStr.replace(enMarker, enInsert);
            Files.write(Paths.get("c:/AGrav/TZAAR/config/statement_en.html"), enStr.getBytes());
        }
        
        String frMarker = "de <var>X = 4</var> \ufffd <var>X = 8</var>.<br>";
        String frStr = new String(Files.readAllBytes(Paths.get("c:/AGrav/TZAAR/config/statement_fr.html")));
        
        // Using regex or exact match due to encoding of à (which is rendered as  or \u00e0)
        String frInsertPattern = "de <var>X = 4</var> .*? <var>X = 8</var>\\.<br>";
        java.util.regex.Matcher m = java.util.regex.Pattern.compile(frInsertPattern).matcher(frStr);
        if (m.find() && !frStr.contains("<details><summary>Afficher la configuration initiale</summary>")) {
            String match = m.group(0);
            String frInsert = match + "\n        <details><summary>Afficher la configuration initiale</summary>\n        <div style=\"text-align: center;\">\n" + svg + "\n        </div>\n        </details>";
            frStr = frStr.replace(match, frInsert);
            Files.write(Paths.get("c:/AGrav/TZAAR/config/statement_fr.html"), frStr.getBytes());
        }
    }
}
