#pragma once
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class HomeWindow : public QWidget {
  Q_OBJECT
public:
  explicit HomeWindow(QWidget *parent = nullptr);

signals:
  // Signaux pour dire à la fenêtre principale quoi faire
  void startInstantSimulation();
  void startRealTimeSimulation();

private:
  QPushButton *btn_instant_;
  QPushButton *btn_realtime_;
};