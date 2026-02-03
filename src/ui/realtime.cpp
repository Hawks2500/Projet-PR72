#include "ui/realtime.h"
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSpinBox>
#include <QSplitter>
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

  patients_snapshots_ = sim.get_patients();

  // Cela mélange programmes et urgences selon leur ordre d'apparition réel
  std::sort(patients_snapshots_.begin(), patients_snapshots_.end(),
            [](const Patient &a, const Patient &b) {
              return a.arrival_time < b.arrival_time;
            });

  // On initialise le tableau (lignes vides)
  table_patients_->setRowCount(patients_snapshots_.size());
  for (size_t i = 0; i < patients_snapshots_.size(); ++i) {
    const auto &p = patients_snapshots_[i];

    // Colonne ID
    table_patients_->setItem(i, 0, new QTableWidgetItem(QString::number(p.id)));

    // Colonne Type (Avec couleur)
    auto *itemType = new QTableWidgetItem(
        p.type == PatientType::Urgent ? "URGENCE" : "Programmé");
    if (p.type == PatientType::Urgent)
      itemType->setForeground(QColor("#ef4444")); // Rouge
    table_patients_->setItem(i, 1, itemType);

    // Colonne Arrivée
    table_patients_->setItem(
        i, 2,
        new QTableWidgetItem(QString::number(p.arrival_time, 'f', 1) + " min"));

    // Colonne État (Vide au début)
    table_patients_->setItem(i, 3, new QTableWidgetItem("Pas encore arrivé"));

    // Colonne Retard (Vide au début)
    table_patients_->setItem(i, 4, new QTableWidgetItem("-"));
  }

  // Optionnel : On s'assure que les évènements sont bien triés par ordre
  // chronologique (Normalement le moteur le fait déjà, mais c'est plus sûr)
  std::sort(
      events_queue_.begin(), events_queue_.end(),
      [](const EventLog &a, const EventLog &b) { return a.time < b.time; });

  // --- AJOUT : DÉTECTION DE LA FIN RÉELLE ---
  if (!events_queue_.empty()) {
    // Le dernier évènement de la liste nous donne l'heure de fin absolue
    double dernier_evenement = events_queue_.back().time;

    // La fin effective est le max entre l'horizon (8h) et le dernier évènement
    // On ajoute un petit "buffer" de 10 minutes pour que ce soit joli
    fin_effective_minutes_ =
        std::max(horizon_minutes_, dernier_evenement + 10.0);
  } else {
    fin_effective_minutes_ = horizon_minutes_;
  }

  // La barre va maintenant de 0 jusqu'à la fin réelle (ex: 10h)
  barre_progression_->setRange(0, static_cast<int>(fin_effective_minutes_));

  log_console_->appendPlainText(
      QString(">>> Scénario généré : %1 évènements prêts à être joués.")
          .arg(events_queue_.size()));

  mettre_a_jour_kpi();
}

QFrame *RealTimeWindow::creer_kpi_widget(const QString &titre,
                                         QLabel *&label_valeur,
                                         const QString &couleur) {
  auto *frame = new QFrame();
  frame->setObjectName("kpiCard"); // Réutilise le style CSS existant
  frame->setStyleSheet(
      QString("QFrame#kpiCard {"
              "  background-color: white;"
              "  border: 1px solid #cbd5e1;" // Bordure fine grise
              "  border-left: 6px solid %1;" // LA BARRE COLORÉE À GAUCHE
              "  border-radius: 8px;"        // Coins arrondis
              "}")
          .arg(couleur));
  // frame->setProperty("kpiColor", couleur);

  auto *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(10, 10, 10, 10);

  auto *lbl_titre = new QLabel(titre);
  lbl_titre->setStyleSheet("color: #64748b; font-size: 8pt; font-weight: bold; "
                           "text-transform: uppercase;");

  label_valeur = new QLabel("0");
  label_valeur->setStyleSheet(
      QString("color: %1; font-size: 20pt; font-weight: 800;").arg(couleur));
  label_valeur->setAlignment(Qt::AlignRight);

  layout->addWidget(lbl_titre);
  layout->addWidget(label_valeur);
  return frame;
}

void RealTimeWindow::basculer_edition_inputs(bool actif) {
  input_horizon_->setEnabled(actif);
  input_salles_->setEnabled(actif);
  input_chirurgiens_->setEnabled(actif);
  input_lits_->setEnabled(actif);
  input_patients_->setEnabled(actif);
  input_urgences_->setEnabled(actif);
}

void RealTimeWindow::mettre_a_jour_kpi() {
  int count_attente = 0;
  int count_bloc = 0;
  int count_reveil = 0;
  int count_sortis = 0;

  // Nouveaux compteurs
  int count_retard = 0;
  int count_annule = 0;

  for (const auto &p : patients_snapshots_) {
    // Patient pas encore arrivé : on ignore
    if (temps_actuel_minutes_ < p.arrival_time)
      continue;

    // 1. CALCUL DES ÉTATS (Attente, Bloc, Réveil, Sorti)
    if (p.start_surgery_time < 0) {
      // Cas spécial : Le patient est annulé
      // On le compte comme "Annulé" dès qu'il est "arrivé" dans la simu
      count_annule++;
      continue; // On ne le compte pas dans "attente" ou autres
    } else if (temps_actuel_minutes_ < p.start_surgery_time) {
      count_attente++;
    } else if (temps_actuel_minutes_ >= p.start_surgery_time &&
               temps_actuel_minutes_ < p.end_surgery_time) {
      count_bloc++;
    } else if (temps_actuel_minutes_ >= p.end_surgery_time &&
               temps_actuel_minutes_ < p.end_recovery_time) {
      count_reveil++;
    } else {
      count_sortis++;
    }

    // 2. CALCUL DU RETARD (> 15 min)
    // Le retard se calcule indépendamment de l'état actuel (c'est un cumulatif)
    if (p.start_surgery_time >= 0) {
      // Si l'opération a commencé (ou est finie), on connait le retard exact
      if (temps_actuel_minutes_ >= p.start_surgery_time) {
        if ((p.start_surgery_time - p.arrival_time) > 15.0) {
          count_retard++;
        }
      }
      // Si l'opération n'a PAS commencé (il attend), on regarde s'il attend
      // DEPUIS trop longtemps
      else {
        if ((temps_actuel_minutes_ - p.arrival_time) > 15.0) {
          count_retard++;
        }
      }
    }
  }

  // Mise à jour des textes
  kpi_attente_->setText(QString::number(count_attente));
  kpi_au_bloc_->setText(QString::number(count_bloc));
  kpi_en_reveil_->setText(QString::number(count_reveil));
  kpi_sortis_->setText(QString::number(count_sortis));

  // Mise à jour des nouveaux KPIs
  kpi_retard_->setText(QString::number(count_retard));
  kpi_annule_->setText(QString::number(count_annule));
}

void RealTimeWindow::construire_ui() {
  // 1. LAYOUT PRINCIPAL HORIZONTAL (Racine)
  // Cela permet d'avoir la barre latérale à gauche et le reste à droite
  auto *main_layout = new QHBoxLayout(this);
  main_layout->setContentsMargins(20, 20, 20, 20);
  main_layout->setSpacing(20);

  // ============================================================
  // COLONNE GAUCHE : CONFIGURATION & KPIs
  // ============================================================
  auto *left_panel = new QWidget();
  left_panel->setFixedWidth(320); // Largeur fixe
  auto *left_layout = new QVBoxLayout(left_panel);
  left_layout->setContentsMargins(0, 0, 0, 0);
  left_layout->setSpacing(15);

  // --- CARTE CONFIGURATION ---
  auto *config_card = creer_carte(this);
  auto *config_layout = new QVBoxLayout(config_card);

  auto *titre_config = new QLabel("Paramètres", config_card);
  titre_config->setObjectName("sectionTitle");
  config_layout->addWidget(titre_config);

  auto *form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignLeft);

  // Initialisation des champs
  input_horizon_ = new QDoubleSpinBox();
  input_horizon_->setRange(1, 48);
  input_horizon_->setValue(8.0);
  input_horizon_->setSuffix(" h");
  form->addRow("Horizon :", input_horizon_);

  input_salles_ = new QSpinBox();
  input_salles_->setRange(1, 20);
  input_salles_->setValue(2);
  form->addRow("Salles Op :", input_salles_);

  input_chirurgiens_ = new QSpinBox();
  input_chirurgiens_->setRange(1, 50);
  input_chirurgiens_->setValue(3);
  form->addRow("Chirurgiens :", input_chirurgiens_);

  input_lits_ = new QSpinBox();
  input_lits_->setRange(1, 50);
  input_lits_->setValue(3);
  form->addRow("Lits Réveil :", input_lits_);

  input_patients_ = new QSpinBox();
  input_patients_->setRange(0, 200);
  input_patients_->setValue(10);
  form->addRow("Programmes :", input_patients_);

  input_urgences_ = new QDoubleSpinBox();
  input_urgences_->setRange(0, 20);
  input_urgences_->setValue(2.0);
  input_urgences_->setSuffix(" /h");
  form->addRow("Taux Urgences :", input_urgences_);

  config_layout->addLayout(form);
  left_layout->addWidget(config_card);

  // --- CARTE KPIs VIVANTS ---
  auto *kpi_group = new QWidget();
  auto *kpi_layout = new QVBoxLayout(kpi_group);
  kpi_layout->setContentsMargins(0, 0, 0, 0);
  kpi_layout->setSpacing(10);

  kpi_layout->addWidget(creer_kpi_widget("En Attente", kpi_attente_, "orange"));
  kpi_layout->addWidget(creer_kpi_widget("Au Bloc", kpi_au_bloc_, "blue"));
  kpi_layout->addWidget(
      creer_kpi_widget("En Réveil", kpi_en_reveil_, "purple"));
  kpi_layout->addWidget(creer_kpi_widget("Sortis", kpi_sortis_, "green"));
  kpi_layout->addWidget(
      creer_kpi_widget("Retards (>15min)", kpi_retard_, "#ef4444")); // Rouge
  kpi_layout->addWidget(creer_kpi_widget("Annulés", kpi_annule_, "#64748b"));

  left_layout->addWidget(kpi_group);
  left_layout->addStretch(); // Pousse tout vers le haut pour coller au sommet

  // AJOUT GAUCHE AU MAIN
  main_layout->addWidget(left_panel);

  // ============================================================
  // COLONNE DROITE : VOTRE INTERFACE EXISTANTE
  // ============================================================
  // On crée un widget conteneur pour tout ce qui était dans votre QVBoxLayout
  // précédent
  auto *right_panel = new QWidget();
  auto *right_layout = new QVBoxLayout(
      right_panel); // C'est ici qu'on remet votre logique verticale
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(20);

  // --- HEADER (VOTRE CODE) ---
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
  header_layout->addStretch();

  right_layout->addWidget(header); // On l'ajoute à la colonne de DROITE

  // --- BARRE DE TEMPS (VOTRE CODE) ---
  auto *timeline_card = creer_carte(this);
  auto *timeline_layout = new QVBoxLayout(timeline_card);

  auto *lbl_progress =
      new QLabel("Progression de la journée (0h - 8h)", timeline_card);
  lbl_progress->setObjectName("blockLabel");

  barre_progression_ = new QProgressBar(timeline_card);
  barre_progression_->setRange(0, static_cast<int>(fin_effective_minutes_));
  barre_progression_->setValue(0);
  barre_progression_->setTextVisible(false);
  barre_progression_->setFixedHeight(25);
  barre_progression_->setStyleSheet(
      "QProgressBar::chunk { background-color: #2563eb; border-radius: 4px; }");

  label_temps_ = new QLabel("00:00", timeline_card);
  label_temps_->setAlignment(Qt::AlignCenter);
  label_temps_->setStyleSheet(
      "font-size: 24pt; font-weight: bold; color: #1e293b;");

  timeline_layout->addWidget(lbl_progress);
  timeline_layout->addWidget(barre_progression_);
  timeline_layout->addWidget(label_temps_);

  right_layout->addWidget(timeline_card); // On l'ajoute à la colonne de DROITE

  // --- ZONE CENTRALE (SPLITTER : LOGS + TABLEAU) (VOTRE CODE) ---
  auto *central_splitter = new QSplitter(Qt::Horizontal, this);
  central_splitter->setChildrenCollapsible(false);

  // 1. Panneau Gauche : Console Logs
  auto *console_container = new QWidget();
  auto *console_layout = new QVBoxLayout(console_container);
  console_layout->setContentsMargins(0, 0, 0, 0);

  auto *lbl_console = new QLabel("Journal des évènements", console_container);
  lbl_console->setObjectName("blockLabel");

  log_console_ = new QPlainTextEdit(console_container);
  log_console_->setObjectName("traceConsole");
  log_console_->setReadOnly(true);

  console_layout->addWidget(lbl_console);
  console_layout->addWidget(log_console_);

  // 2. Panneau Droit : Tableau Patients
  auto *table_container = new QWidget();
  auto *table_layout = new QVBoxLayout(table_container);
  table_layout->setContentsMargins(0, 0, 0, 0);

  auto *lbl_table = new QLabel("État des Patients en Direct", table_container);
  lbl_table->setObjectName("blockLabel");

  table_patients_ = new QTableWidget(table_container);
  table_patients_->setColumnCount(5);
  table_patients_->setHorizontalHeaderLabels(
      {"ID", "Type", "Arrivée", "État Actuel", "Attente/Retard"});

  table_patients_->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  table_patients_->verticalHeader()->setVisible(false);
  table_patients_->setAlternatingRowColors(true);
  table_patients_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Votre style CSS pour le tableau
  table_patients_->setStyleSheet(
      "QTableWidget { background-color: #ffffff; alternate-background-color: "
      "#f8fafc; color: #1e293b; gridline-color: #e2e8f0; border: 1px solid "
      "#cbd5e1; border-radius: 8px; }"
      "QTableWidget::item { padding: 5px; border-bottom: 1px solid #f1f5f9; }"
      "QTableWidget::item:selected { background-color: #e0f2fe; color: "
      "#0c4a6e; }"
      "QHeaderView::section { background-color: #f1f5f9; padding: 6px; border: "
      "none; border-bottom: 2px solid #cbd5e1; font-weight: bold; color: "
      "#475569; }");

  table_layout->addWidget(lbl_table);
  table_layout->addWidget(table_patients_);

  central_splitter->addWidget(console_container);
  central_splitter->addWidget(table_container);
  central_splitter->setStretchFactor(0, 1);
  central_splitter->setStretchFactor(1, 2);

  right_layout->addWidget(central_splitter,
                          1); // Ajout au panneau droit (facteur 1)

  // --- BARRE DE CONTRÔLE (VOTRE CODE) ---
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
  btn_stop_->setObjectName("secondaryButton");
  btn_stop_->setCursor(Qt::PointingHandCursor);
  btn_stop_->setFixedWidth(120);
  btn_stop_->setEnabled(false);

  btn_export_ = new QPushButton("Sauvegarder logs", controls_card);
  btn_export_->setObjectName("secondaryButton");
  btn_export_->setCursor(Qt::PointingHandCursor);
  btn_export_->setFixedWidth(150);
  btn_export_->setEnabled(false);

  controls_layout->addWidget(btn_start_);
  controls_layout->addWidget(btn_pause_);
  controls_layout->addWidget(btn_stop_);
  controls_layout->addWidget(btn_export_);

  right_layout->addWidget(controls_card); // Ajout au panneau droit

  // AJOUT DROITE AU MAIN (Prend toute la place restante)
  main_layout->addWidget(right_panel, 1);

  // --- CONNEXIONS ---
  connect(btn_retour_, &QPushButton::clicked, this, [this]() {
    arreter_simulation();
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

void RealTimeWindow::mettre_a_jour_tableau_patients() {
  for (size_t i = 0; i < patients_snapshots_.size(); ++i) {
    const auto &p = patients_snapshots_[i];

    QTableWidgetItem *itemState = table_patients_->item(i, 3);
    QTableWidgetItem *itemDelay = table_patients_->item(i, 4);

    // --- LOGIQUE D'ÉTAT ---
    // 1. Le patient n'est pas encore là
    if (temps_actuel_minutes_ < p.arrival_time) {
      itemState->setText("Pas arrivé");
      itemState->setForeground(QBrush(QColor("#94a3b8"))); // Gris
      itemDelay->setText("-");
    }
    // 2. Le patient attend (Arrivé mais Chirurgie pas commencée)
    else if (p.start_surgery_time < 0 ||
             temps_actuel_minutes_ < p.start_surgery_time) {
      // Cas spécial : Annulé (si start_surgery_time est -1 et qu'on a dépassé
      // un certain temps ?) Pour l'instant on suppose qu'il attend.
      itemState->setText("EN ATTENTE BLOC");
      itemState->setForeground(QBrush(QColor("#f97316"))); // Orange
      itemState->setFont(QFont("Segoe UI", 9, QFont::Bold));

      // Calcul du retard en temps réel
      double attente = temps_actuel_minutes_ - p.arrival_time;
      itemDelay->setText(QString::number(attente, 'f', 0) + " min");
    }
    // 3. Le patient est au bloc (Entre début et fin chir)
    else if (temps_actuel_minutes_ >= p.start_surgery_time &&
             temps_actuel_minutes_ < p.end_surgery_time) {
      itemState->setText("🔴 AU BLOC OP");
      itemState->setForeground(QBrush(QColor("#2563eb"))); // Bleu vif
      itemState->setFont(QFont("Segoe UI", 9, QFont::Bold));

      // On fige l'attente finale
      double attente_finale = p.start_surgery_time - p.arrival_time;
      itemDelay->setText(QString::number(attente_finale, 'f', 0) + " min");
    }
    // 4. Le patient est en réveil (Entre fin chir et fin réveil)
    else if (temps_actuel_minutes_ >= p.end_surgery_time &&
             temps_actuel_minutes_ < p.end_recovery_time) {
      itemState->setText("🔵 EN RÉVEIL");
      itemState->setForeground(QBrush(QColor("#8b5cf6"))); // Violet
      itemState->setFont(QFont("Segoe UI", 9, QFont::Bold));
    }
    // 5. Le patient est sorti
    else {
      itemState->setText("✅ SORTI");
      itemState->setForeground(QBrush(QColor("#10b981"))); // Vert
      itemState->setFont(QFont("Segoe UI", 9, QFont::Normal));
    }
  }
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
  // --- AJOUT ---
  // On remet le style bleu par défaut
  barre_progression_->setStyleSheet(
      "QProgressBar::chunk { background-color: #2563eb; border-radius: 4px; }");
  label_temps_->setStyleSheet(
      "font-size: 24pt; font-weight: bold; color: #1e293b;");

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
  if (temps_actuel_minutes_ >= fin_effective_minutes_) {
    // arreter_simulation();
    // log_console_->appendPlainText(">>> Fin de la journée.");
    terminer_simulation();
    return;
  }

  // --- AJOUT VISUEL : GESTION DES HEURES SUPPLÉMENTAIRES ---
  if (temps_actuel_minutes_ > horizon_minutes_) {
    // On dépasse les 8h : On change la couleur de la barre en ORANGE ou ROUGE
    // pour bien montrer qu'on fait du "rab".
    barre_progression_->setStyleSheet("QProgressBar::chunk { background-color: "
                                      "#f97316; border-radius: 4px; }" // Orange
    );
    label_temps_->setStyleSheet(
        "font-size: 24pt; font-weight: bold; color: #f97316;");
  } else {
    // Temps normal : Bleu
    barre_progression_->setStyleSheet("QProgressBar::chunk { background-color: "
                                      "#2563eb; border-radius: 4px; }");
    label_temps_->setStyleSheet(
        "font-size: 24pt; font-weight: bold; color: #1e293b;");
  }

  // 3. Mise à jour Interface (Barre + Texte)
  barre_progression_->setValue(static_cast<int>(temps_actuel_minutes_));
  int heures = static_cast<int>(temps_actuel_minutes_ / 60);
  int minutes = static_cast<int>(temps_actuel_minutes_) % 60;
  label_temps_->setText(QString("%1:%2")
                            .arg(heures, 2, 10, QChar('0'))
                            .arg(minutes, 2, 10, QChar('0')));

  // Mise à jour tableau patients
  mettre_a_jour_tableau_patients();

  // Mise à jour KPIs
  mettre_a_jour_kpi();

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