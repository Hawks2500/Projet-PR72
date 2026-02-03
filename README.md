# Simulateur de bloc operatoire AppMed (C++)

Une application de simulation événementielle discrète pour optimiser la gestion d'un bloc opératoire hospitalier. Le projet permet de modéliser le parcours patient (Arrivée -> Attente -> Bloc -> Réveil -> Sortie) avec prise en compte des urgences et des ressources limitées.

L'application propose désormais deux modes : une **Simulation Instantanée** pour les statistiques rapides et un **Monitorage Temps Réel** avec tableau de bord visuel.

## Compilation et execution

```bash
mkdir build
cd build
cmake ..
make

# Lancer l'application
./AppMed

# Lancer les tests unitaires (KPIs, logique moteur)
./tests/test_kpi
```

Prerequis : g++ (C++17) et Qt5 (Widgets) sur Linux/Debian/WSL.

### Interface graphique

- `./build/AppMed` ouvre le menu principal proposant deux modes :
- Simulation Instantanée : Configuration rapide, calcul immédiat et export CSV.
- Simulation Temps Réel : Visualisation "En Direct" de la journée, avec timeline, tableau des patients et KPIs dynamiques.

## Principaux parametres

Les paramètres sont ajustables directement via l'interface graphique (GUI) ou via le terminal :

- `--gui` : lance l'interface graphique Qt.
- `--horizon <heures>` : duree de simulation (arrivees) en heures (defaut 8).
- `--ors <nb>` : nombre de salles d'operation ouvertes (defaut 2).
- `--recovery-beds <nb>` : capacite de la salle de reveil (defaut 3).
- `--elective-count <nb>` et `--elective-window <heures>` : volume d'operations programmees et fenetre d'arrivee.
- `--urgent-rate <par_heure>` : frequence des urgences (arrivees Poisson) en patients/heure.
- `--mean-surgery-elective <minutes>`, `--mean-surgery-urgent <minutes>`, `--mean-recovery <minutes>` : durees moyennes (echantillonnees avec une distribution normale tronquee > 0).
- `--policy <fifo|priority|priorite|balanced|equilibre>` : ordonnanceur pour l'affectation bloc.
  - `fifo` : premier arrive, premier opere.
  - `priority/priorite` : urgences prioritaires, sinon FIFO.
  - `balanced/equilibre` : combine urgence et temps d'attente.
- `--trace` : affiche le journal des evenements.
- `--seed <n>` : graine aleatoire (defaut 1337) pour reproductibilite.

## Sorties

## 1. Dashboard Temps Réel
- Timeline : Barre de progression (Bleue en temps normal, Orange en heures supplémentaires).

- Tableau de suivi : État de chaque patient en direct (En attente, Au bloc, En réveil, Sortie).

  - Gestion des status Retard (>15 min) et Annulé (Hors délais).

- KPIs Vivants : Compteurs dynamiques pour l'occupation, les files d'attente, les retards et les annulations.

## 2. Rapports
Le programme affiche un resume final via une fenêtre de rapport :
- Notation de la performance (Gamification : A, B, C...).
- Graphique de répartition (Opérés vs Annulés).
- Statistiques : volumes d'entrees/sorties, attentes moyennes/maximum, utilisation des blocs et du reveil, debit horaire.
- Logs : Exportation possible de toutes les données en .csv.

## Structure du code

- `src/main.cpp` : Moteur evenementiel, generation des patients, files d'attente, allocation.
- `src/core/` :
  - `simulation.cpp/.h` : Moteur evenementiel, generation des patients, files d'attente, allocation.
  - `patient.cpp/.h` : Structure de données Patient et états.

- `src/ui/` : Interface graphique Qt.
  - `home.cpp` : Menu d'accueil.
  - `gui.cpp` : Fenêtre de configuration (Mode instantané).
  - `realtime.cpp` : Fenêtre de monitorage (Mode temps réel) et logique d'animation.

- `tests/` : Tests unitaires (test_kpi.cpp) validant la logique (Retards, Annulations, Saturation).

- `CMakeLists.txt` : Configuration de la compilation (remplace le Makefile).

