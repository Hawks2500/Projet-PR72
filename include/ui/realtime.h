#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "core/simulation.h"

struct EventLog {
  double time;
  std::string message;
};

class RealTimeWindow : public QWidget {
  Q_OBJECT
public:
  explicit RealTimeWindow(QWidget *parent = nullptr);

signals:
  void retourAccueil();

private slots:
  void demarrer_simulation();
  void mettre_en_pause();
  void arreter_simulation();
  void tic_horloge(); // Appelé par le timer à chaque "tick"

  void terminer_simulation();
  void exporter_logs();

private:
  void construire_ui();
  QFrame *creer_carte(QWidget *parent);

  QFrame *creer_kpi_widget(const QString &titre, QLabel *&label_valeur,
                           const QString &couleur);

  void precalculer_scenario();
  void mettre_a_jour_tableau_patients();

  void mettre_a_jour_kpi();
  void basculer_edition_inputs(bool actif);

  void afficher_rapport_fin();

  // --- PARAMÈTRES (INPUTS) ---
  QDoubleSpinBox *input_horizon_;
  QSpinBox *input_salles_;
  QSpinBox *input_chirurgiens_;
  QSpinBox *input_lits_;
  QSpinBox *input_patients_;
  QDoubleSpinBox *input_urgences_;
  // ---------------------------

  // --- KPIs VIVANTS ---
  QLabel *kpi_attente_;
  QLabel *kpi_au_bloc_;
  QLabel *kpi_en_reveil_;
  QLabel *kpi_sortis_;
  QLabel *kpi_retard_;
  QLabel *kpi_annule_;
  // --------------------

  // Contrôles
  QPushButton *btn_retour_;
  QPushButton *btn_start_;
  QPushButton *btn_pause_;
  QPushButton *btn_stop_;
  QPushButton *btn_export_;

  // Affichage
  QProgressBar *barre_progression_;
  QLabel *label_temps_;
  QPlainTextEdit *log_console_;

  QTableWidget *table_patients_;

  // Logique temporelle
  QTimer *timer_;
  double temps_actuel_minutes_ = 0.0;
  double horizon_minutes_ = 480.0; // 8 heures par défaut
  double fin_effective_minutes_ = 480.0;
  bool en_cours_ = false;

  std::vector<EventLog> events_queue_;
  size_t current_event_index_ = 0;

  std::vector<Patient> patients_snapshots_;
};