import java.nio.file.Files;
import java.nio.file.Paths;
import java.io.IOException;

public class FixSVG2 {
    public static void main(String[] args) throws IOException {
        String imgTag = "<img src=\"board_init.svg\" style=\"width: 100%; max-width: 800px; background: transparent;\" />";
        
        String enStr = new String(Files.readAllBytes(Paths.get("c:/AGrav/TZAAR/config/statement_en.html")), "UTF-8");
        String enPattern = "<details><summary>Show initial board setup</summary>[\\s\\S]*?</details>";
        String enReplacement = "<details><summary>Show initial board setup</summary>\n        <div style=\"text-align: center;\">\n" + imgTag + "\n        </div>\n        </details>";
        enStr = enStr.replaceAll(enPattern, enReplacement);
        Files.write(Paths.get("c:/AGrav/TZAAR/config/statement_en.html"), enStr.getBytes("UTF-8"));
        
        String frStr = new String(Files.readAllBytes(Paths.get("c:/AGrav/TZAAR/config/statement_fr.html")), "UTF-8");
        String frPattern = "<details><summary>Afficher la configuration initiale</summary>[\\s\\S]*?</details>";
        String frReplacement = "<details><summary>Afficher la configuration initiale</summary>\n        <div style=\"text-align: center;\">\n" + imgTag + "\n        </div>\n        </details>";
        frStr = frStr.replaceAll(frPattern, frReplacement);
        Files.write(Paths.get("c:/AGrav/TZAAR/config/statement_fr.html"), frStr.getBytes("UTF-8"));
    }
}
