#include "ui/realtime.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

RealTimeWindow::RealTimeWindow(QWidget *parent) : QWidget(parent) {
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &RealTimeWindow::tic_horloge);
  construire_ui();
}

QFrame *RealTimeWindow::creer_carte(QWidget *parent) {
  auto *frame = new QFrame(parent);
  frame->setObjectName("card");
  return frame;
}

void RealTimeWindow::precalculer_scenario() {
  // 1. On vide la queue précédente
  events_queue_.clear();
  current_event_index_ = 0;

  // 2. On prépare une config (On pourrait ajouter des champs de config sur
  // cette page plus tard)
  SimulationConfig config;
  config.horizon_hours = 8.0;
  config.operating_rooms = 2;
  config.surgeon_count = 3;
  config.recovery_beds = 3;
  config.elective_patients = 12;
  config.trace_events = true; // Important !
  // On met une seed aléatoire pour que chaque "Lecture" soit différente
  config.seed = static_cast<unsigned int>(QDateTime::currentMSecsSinceEpoch());

  // 3. On crée la simulation
  Simulation sim(config);

  // 4. L'ASTUCE EST ICI : On détourne le système de log
  sim.set_log_sink([this](const std::string &msg, double t) {
    // Au lieu d'afficher, on stocke !
    events_queue_.push_back({t, msg});
  });

  // 5. On lance le calcul (c'est instantané)
  sim.run();

  // Optionnel : On s'assure que les évènements sont bien triés par ordre
  // chronologique (Normalement le moteur le fait déjà, mais c'est plus sûr)
  std::sort(
      events_queue_.begin(), events_queue_.end(),
      [](const EventLog &a, const EventLog &b) { return a.time < b.time; });

  log_console_->appendPlainText(
      QString(">>> Scénario généré : %1 évènements prêts à être joués.")
          .arg(events_queue_.size()));
}

void RealTimeWindow::construire_ui() {
  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(24, 24, 24, 24);
  main_layout->setSpacing(20);

  // --- HEADER (Bouton Retour + Titre) ---
  auto *header = creer_carte(this);
  auto *header_layout = new QHBoxLayout(header);

  btn_retour_ = new QPushButton("Accueil", header);
  btn_retour_->setObjectName("backButton");
  btn_retour_->setCursor(Qt::PointingHandCursor);
  btn_retour_->setFixedWidth(100);

  auto *titre = new QLabel("Monitorage Temps Réel", header);
  QFont font = titre->font();
  font.setPointSize(16);
  font.setBold(true);
  titre->setFont(font);
  titre->setAlignment(Qt::AlignCenter);

  header_layout->addWidget(btn_retour_);
  header_layout->addWidget(titre);
  header_layout
      ->addStretch(); // Pousse le titre vers la gauche mais garde le bouton

  // --- BARRE DE TEMPS (Timeline) ---
  auto *timeline_card = creer_carte(this);
  auto *timeline_layout = new QVBoxLayout(timeline_card);

  auto *lbl_progress =
      new QLabel("Progression de la journée (0h - 8h)", timeline_card);
  lbl_progress->setObjectName("blockLabel");

  barre_progression_ = new QProgressBar(timeline_card);
  barre_progression_->setRange(0, static_cast<int>(horizon_minutes_));
  barre_progression_->setValue(0);
  barre_progression_->setTextVisible(false); // On gère le texte nous-mêmes
  barre_progression_->setFixedHeight(25);
  // Style inline pour la barre (ou à mettre dans le CSS)
  barre_progression_->setStyleSheet(
      "QProgressBar::chunk { background-color: #2563eb; border-radius: 4px; }");

  label_temps_ = new QLabel("00:00", timeline_card);
  label_temps_->setAlignment(Qt::AlignCenter);
  label_temps_->setStyleSheet(
      "font-size: 24pt; font-weight: bold; color: #1e293b;");

  timeline_layout->addWidget(lbl_progress);
  timeline_layout->addWidget(barre_progression_);
  timeline_layout->addWidget(label_temps_);

  // --- CONSOLE DE LOGS ---
  auto *console_card = creer_carte(this);
  auto *console_layout = new QVBoxLayout(console_card);

  log_console_ = new QPlainTextEdit(console_card);
  log_console_->setObjectName("traceConsole"); // Réutilise le style noir/bleu
  log_console_->setReadOnly(true);
  log_console_->setPlaceholderText(
      "En attente du lancement de la simulation...");

  console_layout->addWidget(log_console_);

  // --- BARRE DE CONTRÔLE (Play/Pause/Stop) ---
  auto *controls_card = creer_carte(this);
  auto *controls_layout = new QHBoxLayout(controls_card);
  controls_layout->setAlignment(Qt::AlignCenter);
  controls_layout->setSpacing(20);

  btn_start_ = new QPushButton("Lecture", controls_card);
  btn_start_->setObjectName("primaryButton");
  btn_start_->setCursor(Qt::PointingHandCursor);
  btn_start_->setFixedWidth(150);

  btn_pause_ = new QPushButton("Pause", controls_card);
  btn_pause_->setObjectName("secondaryButton");
  btn_pause_->setCursor(Qt::PointingHandCursor);
  btn_pause_->setFixedWidth(120);
  btn_pause_->setEnabled(false);

  btn_stop_ = new QPushButton("Réinitialiser", controls_card);
  btn_stop_->setObjectName(
      "secondaryButton"); // On pourrait faire un style "dangerButton" rouge
  btn_stop_->setCursor(Qt::PointingHandCursor);
  btn_stop_->setFixedWidth(120);
  btn_stop_->setEnabled(false);

  // --- AJOUT BOUTON EXPORT ---
  btn_export_ = new QPushButton("Sauvegarder logs", controls_card);
  btn_export_->setObjectName("secondaryButton");
  btn_export_->setCursor(Qt::PointingHandCursor);
  btn_export_->setFixedWidth(150);
  btn_export_->setEnabled(false); // Désactivé au début

  controls_layout->addWidget(btn_start_);
  controls_layout->addWidget(btn_pause_);
  controls_layout->addWidget(btn_stop_);
  controls_layout->addWidget(btn_export_);

  // Assemblage final
  main_layout->addWidget(header);
  main_layout->addWidget(timeline_card);
  main_layout->addWidget(console_card, 1); // Prend toute la place restante
  main_layout->addWidget(controls_card);

  // --- CONNEXIONS ---
  connect(btn_retour_, &QPushButton::clicked, this, [this]() {
    arreter_simulation(); // On coupe tout avant de partir
    emit retourAccueil();
  });

  connect(btn_start_, &QPushButton::clicked, this,
          &RealTimeWindow::demarrer_simulation);
  connect(btn_pause_, &QPushButton::clicked, this,
          &RealTimeWindow::mettre_en_pause);
  connect(btn_stop_, &QPushButton::clicked, this,
          &RealTimeWindow::arreter_simulation);
  connect(btn_export_, &QPushButton::clicked, this,
          &RealTimeWindow::exporter_logs);
}

// --- LOGIQUE DE SIMULATION ---

void RealTimeWindow::demarrer_simulation() {
  // Si on est au début (t=0), on génère un nouveau scénario
  if (temps_actuel_minutes_ == 0.0) {
    log_console_->clear();
    precalculer_scenario();
  }

  en_cours_ = true;
  timer_->start(50); // Vitesse : 50ms réel = 1 min simulée

  btn_start_->setEnabled(false);
  btn_pause_->setEnabled(true);
  btn_stop_->setEnabled(true);
  btn_export_->setEnabled(false);
}

void RealTimeWindow::mettre_en_pause() {
  en_cours_ = false;
  timer_->stop();

  btn_start_->setText("Reprendre");
  btn_start_->setEnabled(true);
  btn_pause_->setEnabled(false);
  log_console_->appendPlainText(">>> Simulation en pause.");
}

void RealTimeWindow::arreter_simulation() {
  en_cours_ = false;
  timer_->stop();
  temps_actuel_minutes_ = 0;

  // Reset UI
  barre_progression_->setValue(0);
  label_temps_->setText("00:00");
  log_console_->clear();
  log_console_->setPlaceholderText("Simulation réinitialisée.");

  btn_start_->setText("Lecture");
  btn_start_->setEnabled(true);
  btn_pause_->setEnabled(false);
  btn_stop_->setEnabled(false);
  btn_export_->setEnabled(false);
}

void RealTimeWindow::terminer_simulation() {
  en_cours_ = false;
  timer_->stop();

  log_console_->appendPlainText("\n>>> Simulation terminée avec succès.");
  log_console_->appendPlainText(
      ">>> Vous pouvez analyser les logs ci-dessus ou les exporter.");

  // Mise à jour des boutons
  btn_start_->setText("Recommencer"); // Change le texte pour être clair
  btn_start_->setEnabled(
      false); // On oblige à faire "Réinitialiser" pour relancer proprement
  btn_pause_->setEnabled(false);
  btn_stop_->setEnabled(true);   // Le bouton Réinitialiser reste dispo
  btn_export_->setEnabled(true); // On peut sauvegarder !
}

void RealTimeWindow::exporter_logs() {
  // 1. On change le filtre pour .csv
  QString filename = QFileDialog::getSaveFileName(
      this, "Exporter les logs", "simulation_logs.csv", "Fichiers CSV (*.csv)");

  if (filename.isEmpty())
    return;

  QFile file(filename);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, "Erreur", "Impossible d'écrire le fichier.");
    return;
  }

  QTextStream out(&file);

  // Astuce : On force l'encodage UTF-8 avec BOM pour qu'Excel lise bien les
  // accents
  out.setCodec("UTF-8");
  out << "\xEF\xBB\xBF";

  // 2. En-tête du CSV
  out << "Temps (min);Evenement\n";

  // 3. On parcourt notre liste d'évènements en mémoire
  for (const auto &ev : events_queue_) {
    // On n'exporte que les évènements qui se sont DÉJÀ produits
    // (au cas où on exporte pendant une pause)
    if (ev.time <= temps_actuel_minutes_) {

      // On nettoie le message : si le message contient un ";", on le remplace
      // par "," pour ne pas casser la colonne du CSV
      QString message_propre =
          QString::fromStdString(ev.message).replace(";", ",");

      // Format : 12.5;Le patient arrive...
      out << QString::number(ev.time, 'f', 1) << ";" << message_propre << "\n";
    }
  }

  file.close();
  QMessageBox::information(this, "Succès", "Exportation CSV réussie !");
}

void RealTimeWindow::tic_horloge() {
  // 1. On avance le temps
  temps_actuel_minutes_ += 1.0;

  // 2. Fin de simulation ?
  if (temps_actuel_minutes_ >= horizon_minutes_) {
    // arreter_simulation();
    // log_console_->appendPlainText(">>> Fin de la journée.");
    terminer_simulation();
    return;
  }

  // 3. Mise à jour Interface (Barre + Texte)
  barre_progression_->setValue(static_cast<int>(temps_actuel_minutes_));
  int heures = static_cast<int>(temps_actuel_minutes_ / 60);
  int minutes = static_cast<int>(temps_actuel_minutes_) % 60;
  label_temps_->setText(QString("%1:%2")
                            .arg(heures, 2, 10, QChar('0'))
                            .arg(minutes, 2, 10, QChar('0')));

  // 4. REPLAY DES LOGS
  // On regarde tous les évènements en attente qui ont une heure <= heure
  // actuelle
  while (current_event_index_ < events_queue_.size()) {
    const auto &ev = events_queue_[current_event_index_];

    if (ev.time <= temps_actuel_minutes_) {
      // C'est le moment d'afficher cet évènement !
      QString message_formate = QString("[t=%1 min] %2")
                                    .arg(ev.time, 0, 'f', 1)
                                    .arg(QString::fromStdString(ev.message));

      log_console_->appendPlainText(message_formate);

      // On passe au suivant
      current_event_index_++;
    } else {
      // L'évènement suivant est dans le futur, on arrête pour ce tick
      break;
    }
  }
}