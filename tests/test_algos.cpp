#include "core/simulation.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// Structure pour stocker nos résultats comparatifs
struct AlgoResult {
  std::string nom;
  int operes;
  double attente_moyenne_globale;
  double attente_moyenne_urgence;
  double attente_moyenne_prog;
  int retards_urgences;
  int retards_prog;
};

// Fonction utilitaire pour dessiner une barre ASCII
// Exemple : draw_bar(50, 100, 10) -> "#####     "
std::string draw_bar(double value, double max_value, int width = 20) {
  if (max_value <= 0)
    return std::string(width, ' ');

  int filled = static_cast<int>(std::round((value / max_value) * width));
  filled = std::max(0, std::min(filled, width)); // Clamp

  std::string bar = "[";
  bar += std::string(filled, '#');
  bar += std::string(width - filled, ' ');
  bar += "]";
  return bar;
}

AlgoResult tester_politique(SchedulingPolicy policy,
                            const std::string &nom_policy) {
  // 1. Configuration "Stress Test" (Goulot d'étranglement)
  SimulationConfig config;
  config.horizon_hours = 8.0;
  config.operating_rooms = 1; // 1 Salle
  config.surgeon_count = 1;   // 1 Chirurgien
  config.elective_patients = 10;
  config.urgent_rate_per_hour = 1.5; // ~12 Urgences

  config.mean_surgery_minutes_elective = 45.0;
  config.mean_surgery_minutes_urgent = 30.0;
  config.mean_recovery_minutes = 40.0;
  config.cleaning_time_minutes = 10.0;

  config.seed = 42; // Toujours la même graine pour comparer
  config.policy = policy;
  config.trace_events = false;

  // 2. Exécution
  Simulation sim(config);
  SimulationReport report = sim.run();
  const auto &patients = sim.get_patients();

  // 3. Analyse détaillée
  AlgoResult res;
  res.nom = nom_policy;
  res.operes = report.patients_operated;

  double wait_total_urg = 0;
  double wait_total_prog = 0;
  int count_urg = 0;
  int count_prog = 0;

  res.retards_urgences = 0;
  res.retards_prog = 0;

  for (const auto &p : patients) {
    if (p.start_surgery_time >= 0) { // Si opéré
      double wait = p.start_surgery_time - p.arrival_time;

      if (p.type == PatientType::Urgent) {
        wait_total_urg += wait;
        count_urg++;
        if (wait > 15.0)
          res.retards_urgences++;
      } else {
        wait_total_prog += wait;
        count_prog++;
        if (wait > 15.0)
          res.retards_prog++;
      }
    }
  }

  res.attente_moyenne_globale = report.average_wait_to_surgery;
  res.attente_moyenne_urgence =
      (count_urg > 0) ? (wait_total_urg / count_urg) : 0.0;
  res.attente_moyenne_prog =
      (count_prog > 0) ? (wait_total_prog / count_prog) : 0.0;

  return res;
}

void afficher_tableau_et_graphique(const std::vector<AlgoResult> &resultats) {
  std::cout << "\n============================================================="
               "=============================\n";
  std::cout << " COMPARAISON DES ALGORITHMES (Scénario de Saturation : 1 Salle "
               "/ ~22 Patients)\n";
  std::cout << "==============================================================="
               "===========================\n";

  // TABLEAU NUMÉRIQUE
  std::cout << std::left << std::setw(12) << "ALGORITHME"
            << " | " << std::setw(8) << "OPERÉS"
            << " | " << std::setw(11) << "ATT. GLOB"
            << " | " << std::setw(11) << "ATT. URG"
            << " | " << std::setw(11) << "ATT. PROG"
            << " | " << std::setw(15) << "RETARDS (Urg/Pr)"
            << "\n";
  std::cout << "---------------------------------------------------------------"
               "---------------------------\n";

  double max_wait = 0.0;
  for (const auto &r : resultats) {
    std::cout << std::left << std::setw(12) << r.nom << " | " << std::setw(8)
              << r.operes << " | " << std::setw(8) << std::fixed
              << std::setprecision(1) << r.attente_moyenne_globale << " m"
              << " | " << std::setw(8) << r.attente_moyenne_urgence << " m"
              << " | " << std::setw(8) << r.attente_moyenne_prog << " m"
              << " | " << r.retards_urgences << " / " << r.retards_prog << "\n";

    // On cherche le max pour l'échelle du graphique
    if (r.attente_moyenne_prog > max_wait)
      max_wait = r.attente_moyenne_prog;
  }
  std::cout << "==============================================================="
               "===========================\n";

  // GRAPHIQUE ASCII
  std::cout << "\n VISUALISATION GRAPHIQUE DES TEMPS D'ATTENTE (Barres "
               "proportionnelles)\n";
  std::cout << " Echelle : [##########] = " << std::fixed
            << std::setprecision(0) << max_wait << " min (Max observé)\n\n";

  for (const auto &r : resultats) {
    std::cout << " " << std::left << std::setw(12) << r.nom << " :\n";

    // Barre pour les Urgences
    std::cout << "   URGENCE   "
              << draw_bar(r.attente_moyenne_urgence, max_wait, 30) << " "
              << std::setw(5) << r.attente_moyenne_urgence << " min\n";

    // Barre pour les Programmés
    std::cout << "   PROGRAMME "
              << draw_bar(r.attente_moyenne_prog, max_wait, 30) << " "
              << std::setw(5) << r.attente_moyenne_prog << " min\n";

    std::cout << "\n";
  }
  std::cout << "==============================================================="
               "===========================\n";
}

int main() {
  try {
    std::vector<AlgoResult> resultats;

    resultats.push_back(tester_politique(SchedulingPolicy::Fifo, "FIFO"));
    resultats.push_back(
        tester_politique(SchedulingPolicy::PriorityFirst, "PRIORITÉ"));
    resultats.push_back(
        tester_politique(SchedulingPolicy::Balanced, "ÉQUILIBRÉ"));

    afficher_tableau_et_graphique(resultats);
  } catch (const std::exception &e) {
    std::cerr << "Erreur fatale : " << e.what() << std::endl;
    return 1;
  }

  return 0;
}