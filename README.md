# Project Skeleton — Jeu multijoueur CodinGame

Squelette PRÊT À L'EMPLOI : il contient un mini-jeu de course fonctionnel de
bout en bout (compile, se joue en local, s'exporte sur CodinGame). Pour créer
votre jeu, remplacez la logique pièce par pièce en gardant la structure.

## Structure

```
pom.xml                       SDK 4.7.8 (Java 17), endscreen, runner scope compile
config/config.ini             titre, type=multi, min/max joueurs
config/stub.txt               code par défaut généré pour les joueurs (IDE)
config/Boss.java              IA de la ligue — ATTENTION : classe nommée Player !
config/statement_en.html      énoncé (généré par UpdateStatements.java)
src/main/java/com/codingame/game/
  Referee.java                pattern simultané complet : I/O, timeouts, scores
  Player.java                 getExpectedOutputLines()
  Viewer.java                 viewer vectoriel minimal + notes sprites/setTint
src/main/resources/view/
  config.js                   modules viewer + playerColors épinglées + gameName
  assets/                     vos PNG (doit exister même vide)
src/test/java/Main.java       lanceur local (2 boss)
src/test/resources/log4j2.properties   logs referee/joueurs
COMPILE_RUN_LOCAL.txt         commandes + TOUS les pièges connus
NEW_GAME_CHECKLIST.md         questionnaire de conception d'un nouveau jeu
```

## Démarrage rapide

```powershell
javac config/Boss.java
mvn clean install
mvn exec:exec -D"exec.executable"="java" -D"exec.classpathScope"="test" -D"exec.args"="--add-opens java.base/java.lang=ALL-UNNAMED -cp %classpath Main"
# -> http://localhost:8888/test.html
```

## Méthode conseillée (retour d'expérience)

1. Remplissez NEW_GAME_CHECKLIST.md (règles, I/O, ligues, boss).
2. Écrivez le cœur du jeu en JAVA PUR (sans dépendance SDK) dans un sous-package
   `engine/` : il se teste avec javac/java sans Maven ni réseau, et le Referee
   n'est plus qu'un adaptateur I/O autour. Ajoutez un test qui joue des parties
   boss-vs-boss complètes sur toutes les configs (ligues x nb joueurs).
3. Gardez le boss volontairement naïf mais JAMAIS bloqué : c'est le palier de
   promotion. Anti-blocage utile : plan mémorisé + retry/yield asymétrique par
   id + replanification en évitant les cases occupées.
4. Scoring : évitez les égalités structurelles. Critères lexicographiques
   (ex : tour d'arrivée puis nb d'actions) encodés dans un seul int.
5. Statements EN obligatoires, FR en bonus ; régénérez-les via UpdateStatements.
```
