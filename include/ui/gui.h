#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QWidget>

#include "core/simulation.h"

class SimulationWindow : public QWidget {
public:
  SimulationWindow(QWidget *parent = nullptr);

private:
  void construire_ui();
  void configurer_styles();
  QFrame *creer_carte(QWidget *parent = nullptr) const;
  QFrame *creer_kpi_widget(const QString &titre, QLabel *&label_valeur,
                           const QString &couleur);

  void exporter_csv();

  void lancer_simulation();
  SimulationConfig lire_config() const;
  void afficher_rapport(const SimulationConfig &config,
                        const SimulationReport &report);
  void afficher_trace(const std::string &message, double time);

  QDoubleSpinBox *horizon_;
  QSpinBox *ors_;
  QSpinBox *chirurgiens_;
  QSpinBox *lits_reveil_;
  QSpinBox *nb_programmes_;
  QDoubleSpinBox *fenetre_programmes_;
  QDoubleSpinBox *taux_urgences_;
  QDoubleSpinBox *duree_prog_;
  QDoubleSpinBox *duree_urgence_;
  QDoubleSpinBox *duree_reveil_;

  QLabel *val_operes_ = nullptr;
  QLabel *val_attente_bloc_ = nullptr;
  QLabel *val_occup_bloc_ = nullptr;
  QLabel *val_occup_reveil_ = nullptr;

  QLabel *val_retard_ = nullptr;
  QLabel *val_annule_ = nullptr;

  QComboBox *politique_;
  QPlainTextEdit *sortie_;
  QPlainTextEdit *trace_;
  QPushButton *bouton_simuler_;
  QPushButton *bouton_exporter_;

  QDoubleSpinBox *duree_nettoyage_; // <--- NOUVEAU POINTEUR

  SimulationReport dernier_rapport_;
  SimulationConfig dernier_config_;
  std::vector<Patient> derniers_patients_;
};
