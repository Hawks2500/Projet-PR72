#include "core/simulation.h"
#include <cassert>
#include <cmath>
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
  }
}

int main() {
  test_retards_et_annulations();
  return 0;
}