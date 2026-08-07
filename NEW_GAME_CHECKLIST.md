# 📋 Checklist : Informations requises pour implémenter un nouveau jeu CodinGame

Pour pouvoir coder de A à Z un nouveau jeu multijoueur sur CodinGame en partant du "Project_skeleton", j'attendrai de votre part les informations détaillées suivantes :

## 1. Concept et Règles Fondamentales
- [ ] **Thème / Pitch :** Quel est l'univers du jeu (ex: conquête spatiale, combats d'élémentaires, course de voitures) ?
- [ ] **Type de jeu :** Tour par tour (alterné) ou résolution simultanée (tous les joueurs envoient leurs ordres en même temps) ?
- [ ] **Conditions de victoire :** Comment gagne-t-on (score le plus élevé, dernier survivant, capture d'un objectif) ?
- [ ] **Conditions de défaite :** En dehors du crash du code, qu'est-ce qui provoque une élimination (plus de points de vie, sortie du terrain) ?
- [ ] **Égalités (Tie-breakers) :** Si les joueurs ont le même score à la fin de la partie (ou au tour 200), comment les départager ?

## 2. Le Plateau et les Entités
- [ ] **Le Terrain :** S'agit-il d'une grille (carrée, hexagones), d'un graphe (noeuds et liens) ou d'un espace continu (coordonnées flottantes) ? Quelle est sa taille ?
- [ ] **Système de coordonnées :** Où est l'origine (ex: (0,0) en haut à gauche) ?
- [ ] **Les Entités :** Quels sont les objets en jeu (unités, obstacles, bonus) ?
- [ ] **Propriétés des entités :** De quelles variables ont-elles besoin (ID, position X/Y, points de vie, propriétaire, vitesse) ?

## 3. Le Protocole d'Entrée / Sortie (I/O)
- [ ] **Entrées d'initialisation (Tour 0) :** Quelles variables globales donne-t-on au joueur au tout début (ex: nombre de joueurs, taille de la carte) ?
- [ ] **Entrées de boucle (Chaque tour) :** Quelles informations donne-t-on à chaque tour (ex: score de chacun, liste des entités vivantes) ? L'ordre de ces variables est très important.
- [ ] **Sortie attendue :** Quel est le format exact des commandes du joueur (ex: `MOVE X Y`, `ATTACK ID`, `WAIT`) ?
- [ ] **Validations :** Que se passe-t-il si la commande est valide syntaxiquement mais impossible en jeu (ex: se déplacer dans un mur) ? (Généralement ignorée, ou provoque la mort).

## 4. Les Mécaniques de Ligues (Multi-Leagues)
- [ ] **Combien de ligues :** Bois (Wood), Bronze, Argent (Silver), Or (Gold), Légende ?
- [ ] **Évolution des règles :** Quelle nouvelle mécanique ou complexité s'ajoute à chaque ligue (ex: Bois = terrain 6x6 sans brouillard ; Bronze = terrain 9x9 avec brouillard) ?

## 5. La Résolution du Tour (Referee Logic)
- [ ] **Ordre de résolution :** Dans quel ordre exact les actions s'exécutent-elles (ex: 1. Mouvements, 2. Attaques, 3. Apparition des bonus) ?
- [ ] **Collisions & Conflits :** Que se passe-t-il si deux joueurs ciblent la même case ou s'attaquent mutuellement en même temps ?

## 6. L'Interface Graphique (Viewer)
- [ ] **Assets (Images) :** De quelles images avez-vous besoin (fonds d'écran, sprites des unités, icônes) ?
- [ ] **Animations :** Comment illustrer visuellement les actions (déplacements fluides, explosions, changements de couleur, lasers) ?
- [ ] **HUD / UI :** Quelles informations afficher à l'écran pour les spectateurs (barre de vie au-dessus des unités, score en grand, textes d'action) ?

## 7. Le "Boss" (IA de base)
- [ ] **Comportement du Boss :** Quelle doit être la logique de l'IA par défaut qui va affronter les joueurs en ligue Bois/Bronze pour servir de palier d'évaluation ? Doit-il jouer de façon aléatoire, ou suivre une heuristique simple (ex: toujours avancer vers le joueur) ?


---

## 8. Annexe : pièges connus (retour d'expérience Time Loop Facility Escape)

- [ ] **Version SDK** : vérifier qu'elle EXISTE sur Maven Central (4.2.2 n'a jamais existé). SDK >= 4.5.0 => JDK 17.
- [ ] **runner** dans le pom SANS `<scope>test</scope>` (sinon l'export plateforme échoue : Exporter introuvable).
- [ ] **Boss** : fichier `config/Boss.java`, classe `Player` non-publique (compilé comme code joueur IDE).
- [ ] **Lancement local** : `mvn exec:exec` (jamais exec:java) + `--add-opens java.base/java.lang=ALL-UNNAMED`.
- [ ] **view/assets/** doit exister, même vide.
- [ ] **Jeu simultané** : executer TOUS les joueurs avant de lire les sorties ; try/catch TimeoutException par joueur.
- [ ] **Scores** : plancher pour les éliminés, critères lexicographiques pour éviter les draws structurels ; vérifier que le boss reste battable (sinon la ligue est bloquée).
- [ ] **Budget temps** : somme(exécutions x turnMaxTime) <= 30s (quota moteur).
- [ ] **Cartes symétriques** : symétrie miroir ou rotationnelle stricte pour l'équité ; attention aux impasses créées par des unités immobiles sur des cases-articulations.
- [ ] **Sprites** : dessiner en blanc ce qui doit être teinté (setTint) ; épingler playerColors dans config.js = palette du Viewer.
- [ ] **EndScreenModule** : dépendance pom + import dans view/config.js + setScores(scores, textes) dans onEnd().
