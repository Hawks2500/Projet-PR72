#include "ui/gui.h"

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include <QCheckBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>

#include <iomanip>
#include <sstream>

SimulationWindow::SimulationWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("root");
  construire_ui();
}

QLabel *creer_titre_section(const QString &texte) {
  auto *label = new QLabel(texte);
  label->setObjectName("formHeader"); // Lien avec le CSS
  return label;
}

void SimulationWindow::construire_ui() {
  setWindowTitle("Simulation bloc operatoire");
  setMinimumSize(960, 720);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(20);

  auto *header = creer_carte(this);
  auto *header_layout = new QHBoxLayout(header);
  header_layout->setContentsMargins(16, 12, 16, 12);
  header_layout->setSpacing(20);

  // --- AJOUT DU BOUTON RETOUR ---
  bouton_retour_ = new QPushButton("Accueil", header);
  bouton_retour_->setObjectName("backButton"); // Pour le CSS
  bouton_retour_->setCursor(Qt::PointingHandCursor);
  bouton_retour_->setFixedWidth(100); // Petit bouton discret

  // On l'ajoute en premier dans le layout horizontal
  header_layout->addWidget(bouton_retour_);

  auto *titre = new QLabel("Simulation bloc operatoire", header);
  QFont titre_font = titre->font();
  titre_font.setPointSize(titre_font.pointSize() + 4);
  titre_font.setBold(true);
  titre->setFont(titre_font);

  auto *sous_titre = new QLabel(
      "Ajustez les parametres et comparez les politiques en un coup d'oeil.",
      header);
  sous_titre->setObjectName("subtitle");

  auto *header_text = new QVBoxLayout();
  header_text->setSpacing(2);
  header_text->addWidget(titre);
  header_text->addWidget(sous_titre);

  header_layout->addLayout(header_text);
  header_layout->addStretch();

  layout->addWidget(header);

  auto *contenu = new QHBoxLayout();
  contenu->setSpacing(12);
  layout->addLayout(contenu);

  auto *form_card = creer_carte(this);
  form_card->setMinimumWidth(420);

  auto *form_card_layout = new QVBoxLayout(form_card);
  form_card_layout->setContentsMargins(0, 0, 0, 0);
  form_card_layout->setSpacing(0);

  auto *title_container = new QWidget();
  auto *title_layout = new QVBoxLayout(title_container);
  title_layout->setContentsMargins(20, 20, 20, 10);

  auto *form_title = new QLabel("Parametres", title_container);
  form_title->setObjectName("sectionTitle");
  title_layout->addWidget(form_title);

  form_card_layout->addWidget(title_container);

  // --- ZONE DE DÉFILEMENT (SCROLL AREA) ---
  auto *scroll_area = new QScrollArea(form_card);
  scroll_area->setWidgetResizable(
      true);
  scroll_area->setFrameShape(QFrame::NoFrame); 
  scroll_area->setHorizontalScrollBarPolicy(
      Qt::ScrollBarAlwaysOff); // Pas de scroll horizontal

  // Le widget qui contient le formulaire
  auto *scroll_content = new QWidget();
  scroll_content->setObjectName("scrollContent"); // Pour le CSS si besoin

  auto *scroll_layout = new QVBoxLayout(scroll_content);
  scroll_layout->setContentsMargins(20, 0, 20, 20); // Marges internes
  scroll_layout->setSpacing(10);

  auto *form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  form->setFieldGrowthPolicy(
      QFormLayout::ExpandingFieldsGrow); // <--- Les champs prennent toute la
                                         // place
  form->setVerticalSpacing(12);
  form->setHorizontalSpacing(20); // Plus d'espace entre Label et Champ

  form->addRow(creer_titre_section("Configuration Generale"));

  horizon_ = new QDoubleSpinBox(this);
  horizon_->setRange(0.5, 48.0);
  horizon_->setValue(8.0);
  horizon_->setSuffix(" h");
  horizon_->setDecimals(1);
  form->addRow("Horizon (heures)", horizon_);

  // --- SECTION 2 : RESSOURCES ---
  form->addRow(creer_titre_section("Ressources Hospitalières"));

  ors_ = new QSpinBox(this);
  ors_->setRange(1, 20);
  ors_->setValue(2);
  ors_->setMinimumWidth(35);
  form->addRow("Salles d'operation", ors_);

  chirurgiens_ = new QSpinBox(this);
  chirurgiens_->setRange(1, 50);
  chirurgiens_->setValue(3);
  chirurgiens_->setMinimumWidth(35);
  form->addRow("Chirurgiens disponibles", chirurgiens_);

  lits_reveil_ = new QSpinBox(this);
  lits_reveil_->setRange(1, 50);
  lits_reveil_->setValue(3);
  lits_reveil_->setMinimumWidth(35);
  form->addRow("Lits de reveil", lits_reveil_);

  // --- SECTION 3 : FLUX PATIENTS ---
  form->addRow(creer_titre_section("Flux Patients"));

  nb_programmes_ = new QSpinBox(this);
  nb_programmes_->setRange(0, 200);
  nb_programmes_->setValue(10);
  nb_programmes_->setMinimumWidth(35);
  form->addRow("Patients programmes", nb_programmes_);

  fenetre_programmes_ = new QDoubleSpinBox(this);
  fenetre_programmes_->setRange(0.5, 48.0);
  fenetre_programmes_->setValue(6.0);
  fenetre_programmes_->setSuffix(" h");
  fenetre_programmes_->setDecimals(1);
  fenetre_programmes_->setMinimumWidth(35);
  form->addRow("Fenetre d'arrivee programmes", fenetre_programmes_);

  taux_urgences_ = new QDoubleSpinBox(this);
  taux_urgences_->setRange(0.0, 20.0);
  taux_urgences_->setValue(2.0);
  taux_urgences_->setSuffix(" /h");
  taux_urgences_->setMinimumWidth(35);
  form->addRow("Taux d'urgences (λ)", taux_urgences_);

  // --- SECTION 4 : TEMPS MOYENS ---
  form->addRow(creer_titre_section("Durées Moyennes"));

  duree_prog_ = new QDoubleSpinBox(this);
  duree_prog_->setRange(5.0, 600.0);
  duree_prog_->setValue(60.0);
  duree_prog_->setSuffix(" min");
  duree_prog_->setMinimumWidth(35);
  form->addRow("Duree moy. chirurgie programmee", duree_prog_);

  duree_urgence_ = new QDoubleSpinBox(this);
  duree_urgence_->setRange(5.0, 600.0);
  duree_urgence_->setValue(50.0);
  duree_urgence_->setSuffix(" min");
  duree_urgence_->setMinimumWidth(35);
  form->addRow("Duree moy. chirurgie urgente", duree_urgence_);

  duree_reveil_ = new QDoubleSpinBox(this);
  duree_reveil_->setRange(5.0, 600.0);
  duree_reveil_->setValue(45.0);
  duree_reveil_->setSuffix(" min");
  duree_reveil_->setMinimumWidth(35);
  form->addRow("Duree moy. reveil", duree_reveil_);

  // --- AJOUT DU CHAMP NETTOYAGE ICI ---
  duree_nettoyage_ = new QDoubleSpinBox(this);
  duree_nettoyage_->setRange(0.0, 120.0); // De 0 à 2h de nettoyage
  duree_nettoyage_->setValue(15.0);       // Défaut 15 min
  duree_nettoyage_->setSuffix(" min");
  duree_nettoyage_->setMinimumWidth(35);
  form->addRow("Temps de bionettoyage", duree_nettoyage_);
  // ------------------------------------

  // --- SECTION 5 : STRATEGIE ---
  form->addRow(creer_titre_section("Stratégie"));

  politique_ = new QComboBox(this);
  politique_->setView(
      new QListView(this)); // Utilise QListView standard pour un meilleur rendu
  politique_->addItem(
      "FIFO (Premier arrivé)",
      QVariant::fromValue(static_cast<int>(SchedulingPolicy::Fifo)));
  politique_->addItem(
      "Priorite aux urgences",
      QVariant::fromValue(static_cast<int>(SchedulingPolicy::PriorityFirst)));
  politique_->addItem(
      "Equilibre attente/priorite",
      QVariant::fromValue(static_cast<int>(SchedulingPolicy::Balanced)));
  politique_->setCurrentIndex(1);
  politique_->setMinimumWidth(35);
  form->addRow("Politique bloc", politique_);

  auto *trace_checkbox = new QCheckBox("Activer les traces détaillées", this);
  trace_checkbox->setCursor(Qt::PointingHandCursor);
  trace_checkbox->setMinimumWidth(35);
  form->addRow(trace_checkbox);

  scroll_layout->addLayout(form);
  scroll_area->setWidget(scroll_content);
  form_card_layout->addWidget(scroll_area);


  // --- ZONE BOUTONS (Fixe en bas) ---
  // On met les boutons hors du scroll pour qu'ils soient toujours accessibles
  auto *actions_container = new QWidget();
  auto *actions_layout = new QHBoxLayout(actions_container);
  actions_layout->setContentsMargins(20, 10, 20, 20);
  actions_layout->setSpacing(10);

  bouton_simuler_ = new QPushButton("Lancer la simulation", this);
  bouton_simuler_->setObjectName("primaryButton");
  bouton_simuler_->setCursor(Qt::PointingHandCursor);

  bouton_exporter_ = new QPushButton("Exporter CSV", this);
  bouton_exporter_->setObjectName("secondaryButton");
  bouton_exporter_->setCursor(Qt::PointingHandCursor);
  bouton_exporter_->setEnabled(false); // Désactivé par défaut

  actions_layout->addWidget(bouton_simuler_, 2);
  actions_layout->addWidget(bouton_exporter_, 1);

  form_card_layout->addWidget(actions_container);

  contenu->addWidget(form_card, 1);

  auto *output_card = creer_carte(this);
  auto *output_layout = new QVBoxLayout(output_card);
  output_layout->setContentsMargins(16, 16, 16, 16);
  output_layout->setSpacing(10);

  auto *output_title = new QLabel("Resultats", output_card);
  output_title->setObjectName("sectionTitle");
  output_layout->addWidget(output_title);

  auto *splitter = new QSplitter(Qt::Vertical, output_card);
  splitter->setChildrenCollapsible(false);

  auto *synthese_widget = new QWidget(splitter);
  auto *synthese_layout = new QVBoxLayout(synthese_widget);
  synthese_layout->setContentsMargins(0, 0, 0, 0);
  synthese_layout->setSpacing(10);

  auto *kpi_layout = new QGridLayout();
  kpi_layout->setSpacing(10);
  // Carte 1 : Patients Opérés (Bleu)
  kpi_layout->addWidget(
      creer_kpi_widget("Patients Opérés", val_operes_, "blue"), 0, 0);

  // Carte 2 : Attente Moyenne (Orange)
  kpi_layout->addWidget(
      creer_kpi_widget("Attente Moy. Bloc", val_attente_bloc_, "orange"), 0, 1);

  // Carte 3 : NOUVEAU : Retards (Rouge) en haut à droite
  kpi_layout->addWidget(
      creer_kpi_widget("Opérations Retardées", val_retard_, "red"), 0, 2);

  // Carte 4 : Occupation Bloc (Vert)
  kpi_layout->addWidget(
      creer_kpi_widget("Occupation Bloc", val_occup_bloc_, "green"), 1, 0);

  // Carte 5 : Occupation Réveil (Violet)
  kpi_layout->addWidget(
      creer_kpi_widget("Occupation Réveil", val_occup_reveil_, "purple"), 1, 1);

  // Carte 6 : NOUVEAU : Annulés (Gris sombre) en bas à droite
  kpi_layout->addWidget(
      creer_kpi_widget("Opérations Annulées", val_annule_, "gray"), 1, 2);

  synthese_layout->addLayout(kpi_layout);

  auto *synthese_label = new QLabel("Détails textuels", synthese_widget);
  synthese_label->setObjectName("blockLabel");

  sortie_ = new QPlainTextEdit(synthese_widget);
  sortie_->setObjectName("outputConsole");
  sortie_->setReadOnly(true);
  sortie_->setPlaceholderText("Resultats de simulation...");
  synthese_layout->addWidget(synthese_label);
  synthese_layout->addWidget(sortie_);

  auto *trace_widget = new QWidget(splitter);
  auto *trace_layout = new QVBoxLayout(trace_widget);
  trace_layout->setContentsMargins(0, 0, 0, 0);
  trace_layout->setSpacing(6);
  auto *trace_label = new QLabel("Trace", trace_widget);
  trace_label->setObjectName("blockLabel");
  trace_ = new QPlainTextEdit(trace_widget);
  trace_->setObjectName("traceConsole");
  trace_->setReadOnly(true);
  trace_->setPlaceholderText("Trace des evenements (activee si cochee)");
  trace_layout->addWidget(trace_label);
  trace_layout->addWidget(trace_);

  splitter->addWidget(synthese_widget);
  splitter->addWidget(trace_widget);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 4);

  output_layout->addWidget(splitter);
  contenu->addWidget(output_card, 3);

  connect(bouton_simuler_, &QPushButton::clicked, this,
          [this, trace_checkbox]() {
            QSignalBlocker blocker(trace_);
            if (!trace_checkbox->isChecked()) {
              trace_->clear();
            }
            lancer_simulation();
          });

  connect(bouton_exporter_, &QPushButton::clicked, this,
          &SimulationWindow::exporter_csv);

  connect(bouton_retour_, &QPushButton::clicked, this, [this]() {
    reset_interface();
    emit retourAccueil();
  });

  connect(trace_checkbox, &QCheckBox::toggled, this, [this](bool checked) {
    trace_->setEnabled(checked);
    if (!checked)
      trace_->clear();
  });

  trace_->setEnabled(false);
  setLayout(layout);
}

void SimulationWindow::configurer_styles() {
  // Notez le ":" au début. Cela signifie "chercher dans les ressources
  // intégrées"
  QFile file(":/styles.qss");

  if (file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream stream(&file);
    setStyleSheet(stream.readAll());
    file.close();
  } else {
    qWarning() << "Erreur : Impossible de charger le style :/styles.qss";
  }
}

void SimulationWindow::reset_interface() {
  // 1. Reset des zones de texte
  if (trace_)
    trace_->clear();
  if (sortie_)
    sortie_->clear();

  // 2. Désactiver le bouton export
  bouton_exporter_->setEnabled(false);

  // 3. Remettre les KPI à "zéro" ou "-"
  val_operes_->setText("-");
  val_attente_bloc_->setText("-");
  val_occup_bloc_->setText("-");
  val_occup_reveil_->setText("-");
  val_retard_->setText("-");
  val_annule_->setText("-");

  // 4. Vider les données stockées
  derniers_patients_.clear();
  // On garde la config par défaut (champs spinbox) pour ne pas frustrer
  // l'utilisateur, mais on pourrait aussi les remettre aux valeurs par défaut
  // si voulu.
}

QFrame *SimulationWindow::creer_carte(QWidget *parent) const {
  auto *frame = new QFrame(parent);
  frame->setObjectName("card");
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFrameShadow(QFrame::Plain);
  return frame;
}

QFrame *SimulationWindow::creer_kpi_widget(const QString &titre,
                                           QLabel *&label_valeur,
                                           const QString &couleur) {
  auto *frame = new QFrame();
  frame->setObjectName("kpiCard"); // Pour le CSS
  // On stocke la couleur dans une propriété dynamique pour le CSS
  frame->setProperty("kpiColor", couleur);

  auto *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(15, 15, 15, 15);

  auto *lbl_titre = new QLabel(titre);
  lbl_titre->setObjectName("kpiTitle");

  label_valeur = new QLabel("-");
  label_valeur->setObjectName("kpiValue");
  label_valeur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  layout->addWidget(lbl_titre);
  layout->addWidget(label_valeur);

  return frame;
}

SimulationConfig SimulationWindow::lire_config() const {
  SimulationConfig config;
  config.horizon_hours = horizon_->value();
  config.operating_rooms = ors_->value();
  config.surgeon_count = chirurgiens_->value();
  config.recovery_beds = lits_reveil_->value();
  config.elective_patients = nb_programmes_->value();
  config.elective_window_hours = fenetre_programmes_->value();
  config.urgent_rate_per_hour = taux_urgences_->value();
  config.mean_surgery_minutes_elective = duree_prog_->value();
  config.mean_surgery_minutes_urgent = duree_urgence_->value();
  config.mean_recovery_minutes = duree_reveil_->value();

  config.cleaning_time_minutes =
      duree_nettoyage_->value();

  config.policy =
      static_cast<SchedulingPolicy>(politique_->currentData().toInt());
  config.trace_events = trace_->isEnabled();
  return config;
}

void SimulationWindow::afficher_trace(const std::string &message, double time) {
  std::ostringstream os;
  os << "[t=" << std::fixed << std::setprecision(1) << time << " min] "
     << message;
  trace_->appendPlainText(QString::fromStdString(os.str()));
}

void SimulationWindow::afficher_rapport(const SimulationConfig &config,
                                        const SimulationReport &report) {
  // 1. Mise à jour des cartes KPI (Nombres formatés)
  val_operes_->setText(QString::number(report.patients_operated));

  val_attente_bloc_->setText(
      QString::number(report.average_wait_to_surgery, 'f', 1) + " min");

  double occ_bloc = report.operating_room_utilization * 100.0;
  val_occup_bloc_->setText(QString::number(occ_bloc, 'f', 1) + " %");

  double occ_reveil = report.recovery_bed_utilization * 100.0;
  val_occup_reveil_->setText(QString::number(occ_reveil, 'f', 1) + " %");

  // --- Mise à jour des textes ---
  val_retard_->setText(QString::number(report.operations_delayed));
  val_annule_->setText(QString::number(report.operations_cancelled));

  std::ostringstream os;
  os << "--- Détails complets ---\n";
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
  os << "Utilisation chirurgiens : " << report.surgeon_utilization * 100.0
     << "%\n";
  os << "Debit : " << report.throughput_per_hour << " patients/heure\n";
  sortie_->setPlainText(QString::fromStdString(os.str()));
}

void SimulationWindow::exporter_csv() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Sauvegarder les résultats", "simulation_data.csv",
      "Fichiers CSV (*.csv)");

  if (filename.isEmpty())
    return;

  QFile file(filename);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, "Erreur", "Impossible d'écrire le fichier.");
    return;
  }

  QTextStream out(&file);
  out.setCodec("Windows-1252");
  out.setGenerateByteOrderMark(false);

  // 1. En-tête du fichier (Métadonnées)
  out << "--- RAPPORT DE SIMULATION ---\n";
  out << "Date;" << QDateTime::currentDateTime().toString() << "\n";
  out << "Horizon (h);" << dernier_config_.horizon_hours << "\n";
  out << "Salles;" << dernier_config_.operating_rooms << "\n";
  out << "Chirurgiens;" << dernier_config_.surgeon_count << "\n";
  out << "Politique;"
      << QString::fromStdString(
             scheduling_policy_to_string(dernier_config_.policy))
      << "\n";
  out << "\n";

  // 2. Indicateurs globaux
  out << "--- RESULTATS ---\n";
  out << "Patients Arrives;" << dernier_rapport_.patients_arrived << "\n";
  out << "Patients Operes;" << dernier_rapport_.patients_operated << "\n";
  out << "Retards (>15min);" << dernier_rapport_.operations_delayed << "\n";
  out << "Annulations;" << dernier_rapport_.operations_cancelled << "\n";
  out << "Taux Occupation Bloc;" << dernier_rapport_.operating_room_utilization
      << "\n";
  out << "Taux Occupation Chir;" << dernier_rapport_.surgeon_utilization
      << "\n";
  out << "\n";

  // 3. Liste détaillée des patients (Tableau)
  out << "--- LISTE PATIENTS ---\n";
  out << "ID;Type;Arrivee (min);Debut Chir (min);Fin Chir (min);Attente "
         "(min);Statut\n";

  for (const auto &p : derniers_patients_) {
    out << p.id << ";";
    out << (p.type == PatientType::Urgent ? "Urgent" : "Programme") << ";";
    out << QString::number(p.arrival_time, 'f', 2) << ";";

    if (p.start_surgery_time >= 0) {
      out << QString::number(p.start_surgery_time, 'f', 2) << ";";
      out << QString::number(p.end_surgery_time, 'f', 2) << ";";
      out << QString::number(p.start_surgery_time - p.arrival_time, 'f', 2)
          << ";";
      out << "Operé";
    } else {
      out << "-;-;-;Annulé";
    }
    out << "\n";
  }

  file.close();
  QMessageBox::information(this, "Succès", "Exportation réussie !");
}

void SimulationWindow::lancer_simulation() {
  if (mode_temps_reel_) {
    // Pour l'instant, on affiche juste que le mode est activé
    trace_->appendPlainText(
        ">>> MODE TEMPS RÉEL ACTIVÉ (Simulation visuelle à venir) <<<");
    afficher_trace("Initialisation du mode temps réel...", 0.0);

    return;
  }

  SimulationConfig config = lire_config();
  Simulation simulation(config);

  if (config.trace_events) {
    trace_->clear();
    simulation.set_log_sink(
        [this](const std::string &msg, double t) { afficher_trace(msg, t); });
  }

  SimulationReport report = simulation.run();

  dernier_config_ = config;
  dernier_rapport_ = report;
  derniers_patients_ = simulation.get_patients();
  bouton_exporter_->setEnabled(true);

  afficher_rapport(config, report);
}
