import com.codingame.gameengine.runner.MultiplayerGameRunner;

/**
 * Lanceur local. Voir COMPILE_RUN_LOCAL.txt pour les commandes exactes
 * (ATTENTION : mvn exec:exec, PAS exec:java -> sinon page 404).
 */
public class Main {
    public static void main(String[] args) {
        MultiplayerGameRunner gameRunner = new MultiplayerGameRunner();

        gameRunner.setLeagueLevel(1);
        gameRunner.setSeed(42L);

        // Le boss local : javac config/Boss.java produit config/Player.class
        gameRunner.addAgent("java -cp config Player", "Boss-Alpha");
        gameRunner.addAgent("java -cp config Player", "Boss-Omega");
        // 3e/4e joueur :
        // gameRunner.addAgent("java -cp config Player", "Boss-Gamma");
        // gameRunner.addAgent("java -cp config Player", "Boss-Delta");

        gameRunner.start();   // http://localhost:8888/test.html
    }
}
