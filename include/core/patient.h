#pragma once

enum class PatientType { Elective, Urgent };

struct Patient {
  int id = -1;
  PatientType type = PatientType::Elective;

  // Données temporelles
  double arrival_time = 0.0;
  double surgery_duration = 0.0;
  double recovery_duration = 0.0;

  // Timestamps de suivi (initialisés à -1.0)
  double start_surgery_time = -1.0;
  double end_surgery_time = -1.0;
  double start_recovery_time = -1.0;
  double end_recovery_time = -1.0;

  // Constructeurs
  Patient() = default;
  Patient(int id, PatientType type, double arrival);

  bool is_urgent() const;
  bool is_completed() const;
};