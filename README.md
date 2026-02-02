# Simulateur de bloc operatoire (C++)

Simulation evenementielle discrete du parcours patient dans un bloc operatoire hospitalier : arrivee, attente pre-op, bloc, reveil. Deux types de patients sont generes (programmes et urgents) avec priorisation et files d'attente separees. La simulation calcule des indicateurs clefs (temps d'attente, utilisation des salles, debit).

## Compilation et execution

```bash
make           # compile vers build/simulator
./build/simulator --help

# ou via le script run (recompile si besoin)
./run --trace --policy balanced
```

Prerequis : g++ (C++17) et Qt5 (Widgets) sur Linux/Debian.

### Interface graphique

- `./run --gui` (ou `./build/simulator --gui`) ouvre une fenetre Qt pour saisir les parametres, lancer la simulation et visualiser le resume + une trace d'evenements.
- Les champs couvrent horizon, capacites bloc/reveil, volumes de programmes/urgences, durees moyennes et politique d'ordonnancement.

## Principaux parametres

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

Le programme affiche un resume : volumes d'entrees/sorties, attentes moyennes/maximum, utilisation des blocs et du reveil, debit horaire, patients restants en attente. Avec `--trace`, chaque arrivee/debut/fin est journalise avec son horodatage (minutes depuis t0).

## Structure du code

- `src/main.cpp` : parse les options CLI, lance la simulation, formate les indicateurs.
- `src/simulation.h/.cpp` : moteur evenementiel, generation des patients (electifs + urgents), files d'attente, allocation bloc/reveil, calcul des metriques.
- `Makefile` : compilation simple (g++ -std=c++17).
- `run` : script utilitaire pour builder puis executer.

