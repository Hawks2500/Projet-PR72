#include <QApplication>
#include <QStackedWidget>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/simulation.h"
#include "ui/gui.h"
#include "ui/home.h"

namespace {

void afficher_aide(const char *nom) {
  std::cout
      << "Usage : " << nom << " [options]\n"
      << "Options CLI (resume texte) :\n"
      << "  --horizon <heures>            Duree de simulation (defaut 8)\n"
      << "  --ors <nb>                    Nombre de salles d'operation (defaut "
         "2)\n"
      << "  --recovery-beds <nb>          Nombre de lits de reveil (defaut 3)\n"
      << "  --elective-count <nb>         Nombre de patients programmes "
         "(defaut 10)\n"
      << "  --elective-window <heures>    Fenetre d'arrivee des programmes "
         "(defaut 6)\n"
      << "  --urgent-rate <par_heure>     Frequence des urgences "
         "(patients/heure, defaut 2)\n"
      << "  --mean-surgery-elective <m>   Duree moy. chirurgie programmee "
         "(minutes)\n"
      << "  --mean-surgery-urgent <m>     Duree moy. chirurgie urgente "
         "(minutes)\n"
      << "  --mean-recovery <m>           Duree moy. de reveil (minutes)\n"
      << "  --policy <fifo|priority|priorite|balanced|equilibre> Politique "
         "d'ordonnancement (defaut priority)\n"
      << "  --trace                       Affiche la trace des evenements\n"
      << "  --seed <n>                    Graine aleatoire (defaut 1337)\n"
      << "  --gui                         Lance l'interface graphique\n"
      << "  --help                        Affiche cette aide\n";
}

bool parse_double(const std::string &valeur, double &out) {
  try {
    size_t idx = 0;
    out = std::stod(valeur, &idx);
    return idx == valeur.size();
  } catch (...) {
    return false;
  }
}

bool parse_int(const std::string &valeur, int &out) {
  try {
    size_t idx = 0;
    const long parsed = std::stol(valeur, &idx);
    if (idx != valeur.size() || parsed < 0)
      return false;
    out = static_cast<int>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

SchedulingPolicy parse_policy(const std::string &value) {
  if (value == "fifo")
    return SchedulingPolicy::Fifo;
  if (value == "priority" || value == "priorite")
    return SchedulingPolicy::PriorityFirst;
  if (value == "balanced" || value == "equilibre")
    return SchedulingPolicy::Balanced;
  throw std::invalid_argument("Politique inconnue: " + value);
}

std::string rendre_rapport(const SimulationConfig &config,
                           const SimulationReport &report) {
  std::ostringstream os;
  os << "\n--- Synthese simulation ---\n";
  os << "Politique : " << scheduling_policy_to_string(config.policy)
     << " | Salles : " << config.operating_rooms
     << " | Lits reveil : " << config.recovery_beds << '\n';
  os << "Horizon : " << config.horizon_hours << " h"
     << " | Programmes : " << config.elective_patients
     << " | Urgences (lambda) : " << config.urgent_rate_per_hour << "/h\n";
  os << "Arrives : " << report.patients_arrived
     << " (urgences : " << report.urgent_arrived
     << ", programmes : " << report.elective_arrived << ")\n";
  os << "Operes : " << report.patients_operated
     << " | Termines (apres reveil) : " << report.patients_completed
     << " | En attente bloc : " << report.pending_waiting << '\n';
  os << "Attente moyenne avant bloc : " << report.average_wait_to_surgery
     << " min (max " << report.max_wait_to_surgery << ")\n";
  os << "Attente moyenne vers reveil : " << report.average_wait_to_recovery
     << " min\n";
  os << "Temps moyen dans le systeme : " << report.average_total_time_in_system
     << " min\n";
  os << "Utilisation blocs : " << report.operating_room_utilization * 100.0
     << "% | Utilisation reveil : " << report.recovery_bed_utilization * 100.0
     << "%\n";
  os << "Debit : " << report.throughput_per_hour << " patients/heure\n";
  return os.str();
}

} // namespace

int main(int argc, char *argv[]) {
  SimulationConfig config;
  bool lancer_gui = false;
  std::vector<std::string> args(argv + 1, argv + argc);

  for (size_t i = 0; i < args.size(); ++i) {
    const std::string &arg = args[i];
    auto besoin_valeur = [&](const std::string &flag) -> std::string {
      if (i + 1 >= args.size()) {
        throw std::invalid_argument("Valeur manquante pour " + flag);
      }
      return args[++i];
    };

    try {
      if (arg == "--help") {
        afficher_aide(argv[0]);
        return 0;
      } else if (arg == "--gui") {
        lancer_gui = true;
      } else if (arg == "--horizon") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Horizon invalide");
        }
        config.horizon_hours = value;
      } else if (arg == "--ors") {
        int value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_int(raw, value)) {
          throw std::invalid_argument("Nombre de salles invalide");
        }
        config.operating_rooms = value;
      } else if (arg == "--recovery-beds") {
        int value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_int(raw, value)) {
          throw std::invalid_argument("Nombre de lits invalide");
        }
        config.recovery_beds = value;
      } else if (arg == "--elective-count") {
        int value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_int(raw, value)) {
          throw std::invalid_argument("Nombre de programmes invalide");
        }
        config.elective_patients = value;
      } else if (arg == "--elective-window") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Fenetre programmes invalide");
        }
        config.elective_window_hours = value;
      } else if (arg == "--urgent-rate") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Taux urgence invalide");
        }
        config.urgent_rate_per_hour = value;
      } else if (arg == "--mean-surgery-elective") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Duree chirurgie programmee invalide");
        }
        config.mean_surgery_minutes_elective = value;
      } else if (arg == "--mean-surgery-urgent") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Duree chirurgie urgente invalide");
        }
        config.mean_surgery_minutes_urgent = value;
      } else if (arg == "--mean-recovery") {
        double value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_double(raw, value)) {
          throw std::invalid_argument("Duree reveil invalide");
        }
        config.mean_recovery_minutes = value;
      } else if (arg == "--policy") {
        const std::string raw = besoin_valeur(arg);
        config.policy = parse_policy(raw);
      } else if (arg == "--trace") {
        config.trace_events = true;
      } else if (arg == "--seed") {
        int value;
        const std::string raw = besoin_valeur(arg);
        if (!parse_int(raw, value)) {
          throw std::invalid_argument("Graine invalide");
        }
        config.seed = static_cast<unsigned int>(value);
      } else {
        throw std::invalid_argument("Option inconnue : " + arg);
      }
    } catch (const std::exception &ex) {
      std::cerr << "Erreur : " << ex.what() << "\n";
      afficher_aide(argv[0]);
      return 1;
    }
  }

  for (const auto &arg : args) {
    if (arg == "--gui") {
      lancer_gui = true;
    }
  }

  if (lancer_gui || args.empty()) {
    QApplication app(argc, argv);

    QFile styleFile(":/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
      QString style = QLatin1String(styleFile.readAll());
      app.setStyleSheet(style); // Applique le style à TOUTE l'application
    }

    QStackedWidget *stackedWidget = new QStackedWidget();
    stackedWidget->setObjectName("root"); // Fond dégradé global
    stackedWidget->resize(1280, 800);
    stackedWidget->setWindowTitle("Simulateur Bloc Opératoire");

    // Création des pages
    HomeWindow *homePage = new HomeWindow();
    SimulationWindow *simPage = new SimulationWindow();

    // Ajout à la pile
    stackedWidget->addWidget(homePage); // Index 0
    stackedWidget->addWidget(simPage);  // Index 1

    // Logique de navigation (Connexion des signaux)
    QObject::connect(homePage, &HomeWindow::startInstantSimulation, [=]() {
      simPage->set_mode_temps_reel(false); // Mode classique
      stackedWidget->setCurrentIndex(1);   // Aller à la page simu
    });

    QObject::connect(homePage, &HomeWindow::startRealTimeSimulation, [=]() {
      simPage->set_mode_temps_reel(true); // Mode temps réel
      stackedWidget->setCurrentIndex(1);  // Aller à la page simu
    });

    QObject::connect(simPage, &SimulationWindow::retourAccueil, [=]() {
      stackedWidget->setCurrentIndex(0); // Retour à l'accueil
    });

    stackedWidget->show();
    return app.exec();
    // SimulationWindow fenetre;
    // fenetre.show();
    // return app.exec();
  }

  Simulation simulation(config);
  SimulationReport report = simulation.run();
  std::cout << rendre_rapport(config, report);
  return 0;
}
