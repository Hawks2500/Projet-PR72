#include "ui/home.h"
#include <QFrame>
#include <QGraphicsDropShadowEffect>

HomeWindow::HomeWindow(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setAlignment(Qt::AlignCenter);
  layout->setSpacing(30);

  // --- TITRE ---
  auto *title = new QLabel("Simulateur Bloc Opératoire", this);
  title->setObjectName("appTitle"); // Pour le CSS
  title->setAlignment(Qt::AlignCenter);

  auto *subtitle = new QLabel("Choisissez votre mode de simulation", this);
  subtitle->setObjectName("appSubtitle");
  subtitle->setAlignment(Qt::AlignCenter);

  // --- CONTENEUR DES BOUTONS (CARTE) ---
  auto *card = new QFrame(this);
  card->setObjectName("card"); // Style carte blanche
  card->setFixedSize(500, 300);

  auto *card_layout = new QVBoxLayout(card);
  card_layout->setSpacing(20);
  card_layout->setContentsMargins(40, 40, 40, 40);

  // Bouton Instantané
  btn_instant_ = new QPushButton("Simulation Instantanée", this);
  btn_instant_->setObjectName("primaryButton"); // Style bleu
  btn_instant_->setCursor(Qt::PointingHandCursor);
  btn_instant_->setFixedHeight(60);

  // Bouton Temps Réel
  btn_realtime_ = new QPushButton("Simulation Temps Réel (Visuel)", this);
  btn_realtime_->setObjectName("secondaryButton"); // Style blanc
  btn_realtime_->setCursor(Qt::PointingHandCursor);
  btn_realtime_->setFixedHeight(60);

  card_layout->addStretch();
  card_layout->addWidget(btn_instant_);
  card_layout->addWidget(btn_realtime_);
  card_layout->addStretch();

  // Ajout au layout principal
  layout->addStretch();
  layout->addWidget(title);
  layout->addWidget(subtitle);
  layout->addWidget(card);
  layout->addStretch();

  // --- CONNEXIONS ---
  connect(btn_instant_, &QPushButton::clicked, this,
          &HomeWindow::startInstantSimulation);
  connect(btn_realtime_, &QPushButton::clicked, this,
          &HomeWindow::startRealTimeSimulation);
}