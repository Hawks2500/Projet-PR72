#include "core/simulation.h"
#include <cassert>
#include <cmath>
<<<<<<< HEAD
#include <iostream>
#include <string>

// Une fonction simple pour vérifier une condition et afficher le résultat
void assert_test(bool condition, const std::string &message) {
  if (condition) {
    std::cout << "[OK] " << message << std::endl;
  } else {
    std::cout << "[FAIL] " << message << std::endl;
    std::exit(1); // Arrête le test immédiatement en cas d'erreur
  }
}

void test_retards_et_annulations() {
  std::cout << "--- Test KPI: Retards et Annulations ---" << std::endl;

  SimulationConfig config;

  // 1. Configuration pour SATURER le bloc
  config.seed = 42; // Graine fixe pour avoir toujours le même résultat
  config.horizon_hours = 10.0; // Simulation de 10 heures
  config.operating_rooms = 1;  // Une seule salle (goulot d'étranglement)

  // 2. Beaucoup de patients programmés qui arrivent en même temps
  config.elective_patients = 10; // 10 patients
  config.elective_window_hours =
      1.0; // Ils arrivent tous dans la 1ère heure (t=0 à t=60)

  // 3. Opérations très longues
  config.mean_surgery_minutes_elective = 120.0; // 2 heures par opération !

  // 4. IMPORTANT : Désactiver les urgences
  // Si on laisse les urgences, elles passent devant grâce à la priorité.
  // Les patients programmés restent alors bloqués indéfiniment et finissent
  // "Annulés" sans jamais être "Retardés". En mettant 0, on force le bloc à
  // traiter la file d'attente standard.
  config.urgent_rate_per_hour = 0.0;

  // 5. Pas de nettoyage pour simplifier le calcul mental
  config.cleaning_time_minutes = 0.0;

  // Lancement de la simulation
  Simulation sim(config);
  SimulationReport report = sim.run();

  // Affichage des résultats bruts pour debug
  std::cout << "Resultats obtenus :" << std::endl;
  std::cout << " - Arrives  : " << report.patients_arrived << std::endl;
  std::cout << " - Operes   : " << report.patients_operated << std::endl;
  std::cout << " - Retardes : " << report.operations_delayed << std::endl;
  std::cout << " - Annules  : " << report.operations_cancelled << std::endl;

  // --- Vérifications Logiques ---

  // A. Test des Annulations
  // Calcul théorique :
  // Capacité max = 10h (horizon) / 2h (durée op) = 5 patients max.
  // 10 patients arrivent. Donc environ 5 doivent rester sur le carreau
  // (Annulés).
  assert_test(report.operations_cancelled > 0,
              "Il doit y avoir des operations annulees (saturation)");
  assert_test(report.pending_waiting == report.operations_cancelled,
              "Pending waiting == Cancelled");

  // B. Test des Retards
  // Le Patient 1 arrive vers t=0, entre au bloc vers t=0. Attente ~0 min. (Pas
  // de retard) Le Patient 2 arrive vers t=6, mais le bloc est occupé jusqu'à
  // t=120 (fin du P1). Le Patient 2 entre au bloc à t=120. Attente = 120 - 6 =
  // 114 min. 114 min > 15 min (seuil de retard). Donc il DOIT être compté comme
  // retardé.

  if (report.patients_operated >= 2) {
    assert_test(report.operations_delayed > 0,
                "Il doit y avoir des retards (le 2eme patient attend le 1er)");
  } else {
    std::cout << "[INFO] Pas assez de patients operes pour tester le retard "
                 "(ce n'est pas normal avec ce seed)"
              << std::endl;
=======
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// --- UTILITAIRES ---

void print_header(const std::string &title) {
  std::cout << "\n========================================\n";
  std::cout << " TEST : " << title << "\n";
  std::cout << "========================================\n";
}

void assert_test(bool condition, const std::string &message) {
  if (condition) {
    std::cout << " [OK] " << message << std::endl;
  } else {
    std::cout << " [FAIL] " << message << std::endl;
    std::exit(1);
  }
}

// --- SCÉNARIO 1 : JOURNÉE IDEALE (Tout doit passer) ---
void test_journee_ideale() {
  print_header("Journee Ideale (Pas de saturation)");

  SimulationConfig config;
  config.seed = 100;
  config.horizon_hours = 10.0;  // 10h d'ouverture
  config.operating_rooms = 2;   // 2 salles
  config.elective_patients = 8; // 8 patients (4 par salle, c'est large)
  config.mean_surgery_minutes_elective = 60.0; // 1h par op
  config.cleaning_time_minutes = 15.0;         // 15min ménage
  config.urgent_rate_per_hour = 0.0;           // Pas d'urgences

  Simulation sim(config);
  SimulationReport report = sim.run();

  std::cout << " -> Operes : " << report.patients_operated << "/"
            << config.elective_patients << "\n";
  std::cout << " -> Annules : " << report.operations_cancelled << "\n";

  // Vérifications
  assert_test(report.patients_operated == 8,
              "Tous les patients doivent etre operes");
  assert_test(report.operations_cancelled == 0, "Aucune annulation prevue");
  assert_test(report.operating_room_utilization < 0.9,
              "Le bloc ne doit pas etre sature (< 90%)");
}

// --- SCÉNARIO 2 : SATURATION & ANNULATIONS (Le test critique) ---
void test_saturation_realiste() {
  print_header("Saturation du Bloc (Test de coupure d'horizon)");

  SimulationConfig config;
  config.seed = 42;
  config.horizon_hours = 8.0; // 8h d'ouverture (480 min)
  config.operating_rooms = 2; // 2 salles

  // CAS DE FIGURE :
  // Capacité théorique par salle = 480 min.
  // Temps par patient = 60 min (op) + 15 min (clean) = 75 min.
  // Max patients par salle = 480 / 75 = 6.4 -> 6 patients max.
  // Total max possible = 12 patients.

  // On envoie 20 patients ! Il doit y avoir de la casse.
  config.elective_patients = 20;
  config.mean_surgery_minutes_elective = 60.0;
  config.cleaning_time_minutes = 15.0;
  config.elective_window_hours = 2.0; // Ils arrivent tous le matin (bouchon)
  config.urgent_rate_per_hour = 0.0;

  Simulation sim(config);
  SimulationReport report = sim.run();

  std::cout << " -> Arrives : " << report.patients_arrived << "\n";
  std::cout << " -> Operes : " << report.patients_operated
            << " (Capacite max theorique ~12)\n";
  std::cout << " -> Annules : " << report.operations_cancelled << "\n";
  std::cout << " -> Retardes (>15min) : " << report.operations_delayed << "\n";

  // Vérifications logiques
  assert_test(report.patients_operated <= 14,
              "Le nombre d'operes doit etre realiste (max ~12-13)");
  assert_test(report.operations_cancelled >= 5,
              "Il doit y avoir au moins 5 annulations (20 demandés - 13 max)");
  assert_test(
      report.operations_delayed > 5,
      "Avec un tel bouchon, beaucoup de patients doivent etre en retard");
  assert_test(report.operating_room_utilization > 0.85,
              "Le bloc doit tourner a plein regime (> 85%)");
}

// --- SCÉNARIO 3 : IMPACT DES URGENCES (Priorité) ---
void test_priorite_urgence() {
  print_header("Impact des Urgences (Test Priorite)");

  SimulationConfig config;
  config.seed = 999;
  config.horizon_hours = 12.0;
  config.operating_rooms = 1; // 1 seule salle pour forcer le conflit

  // Un programme chargé
  config.elective_patients = 5;
  config.mean_surgery_minutes_elective = 60.0;

  // Beaucoup d'urgences qui arrivent
  config.urgent_rate_per_hour = 0.5; // Une urgence toutes les 2h env.
  config.mean_surgery_minutes_urgent = 60.0;

  config.policy =
      SchedulingPolicy::PriorityFirst; // Les urgences passent devant !

  Simulation sim(config);
  SimulationReport report = sim.run();

  std::cout << " -> Total Operes : " << report.patients_operated << "\n";
  std::cout << " -> Urgences Operees : " << report.urgent_arrived << "\n";
  std::cout << " -> Attente Moyenne : " << report.average_wait_to_surgery
            << " min\n";
  std::cout << " -> Retards : " << report.operations_delayed << "\n";

  // Vérification :
  // S'il y a des urgences, elles ont dû retarder les programmés.
  // Avec 1 salle et des urgences qui s'intercalent, l'attente moyenne doit
  // exploser.

  if (report.urgent_arrived > 0) {
    assert_test(report.operations_delayed > 0,
                "Les urgences doivent creer des retards pour les programmes");
  } else {
    std::cout << " [INFO] Pas assez d'urgences generees avec cette graine pour "
                 "tester le conflit.\n";
>>>>>>> 45492c1 (Fix git de merde)
  }
}

int main() {
<<<<<<< HEAD
  test_retards_et_annulations();
  return 0;
=======
  try {
    test_journee_ideale();
    test_saturation_realiste();
    test_priorite_urgence();

    std::cout << "\n========================================\n";
    std::cout << " TOUS LES TESTS SONT PASSES AVEC SUCCES \n";
    std::cout << "========================================\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Erreur fatale : " << e.what() << std::endl;
    return 1;
  }
>>>>>>> 45492c1 (Fix git de merde)
}