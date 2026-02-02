#pragma once

#include <deque>
#include <functional>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "core/patient.h"

enum class EventType { Arrival, SurgeryEnd, CleaningEnd, RecoveryEnd };

enum class SchedulingPolicy { Fifo, PriorityFirst, Balanced };

struct SimulationConfig {
  double horizon_hours = 8.0;
  int operating_rooms = 2;
  int recovery_beds = 3;
  int elective_patients = 10;

  int surgeon_count = 3;

  double elective_window_hours = 6.0;
  double urgent_rate_per_hour = 2.0;
  double cleaning_time_minutes = 15.0;

  double mean_surgery_minutes_elective = 60.0;
  double mean_surgery_minutes_urgent = 50.0;
  double mean_recovery_minutes = 45.0;

  SchedulingPolicy policy = SchedulingPolicy::PriorityFirst;
  bool trace_events = false;
  unsigned int seed = 1337u;
};

struct Event {
  double time = 0.0;
  EventType type = EventType::Arrival;
  int patient_id = -1;
};

struct SimulationReport {
  int patients_arrived = 0;
  int urgent_arrived = 0;
  int elective_arrived = 0;
  int patients_operated = 0;
  int patients_completed = 0;
  double average_wait_to_surgery = 0.0;
  double average_wait_to_recovery = 0.0;
  double average_total_time_in_system = 0.0;
  double max_wait_to_surgery = 0.0;
  double operating_room_utilization = 0.0; // ratio [0,1]
  double recovery_bed_utilization = 0.0;   // ratio [0,1]

  double surgeon_utilization = 0.0; // Taux d'occupation des chirurgiens

  double throughput_per_hour = 0.0;
  int pending_waiting = 0;

  int operations_delayed = 0;
  int operations_cancelled = 0;
};

class Simulation {
public:
  explicit Simulation(SimulationConfig config);
  SimulationReport run();
  void set_log_sink(std::function<void(const std::string &, double)> sink);

  const std::vector<Patient> &get_patients() const { return patients_; }

private:
  using EventQueue =
      std::priority_queue<Event, std::vector<Event>,
                          std::function<bool(const Event &, const Event &)>>;

  void seed_patients();
  void push_event(const Event &event);

  void handle_arrival(const Event &event, double now);
  void handle_surgery_end(const Event &event, double now);
  void handle_cleaning_end(const Event &event, double now);
  void handle_recovery_end(const Event &event, double now);

  void try_schedule_surgery(double now);
  void try_start_recovery(double now);

  int pick_next_patient(double now);

  double draw_positive_duration(double mean_minutes);
  double draw_urgent_interarrival_minutes();

  void log_event(const std::string &message, double time);

  SimulationConfig config_;
  std::function<bool(const Event &, const Event &)> event_compare_;
  EventQueue events_;
  std::function<void(const std::string &, double)> log_sink_;
  std::vector<Patient> patients_;
  std::vector<int> waiting_patients_; // patient ids waiting for OR
  std::deque<int> recovery_waiting_;  // patient ids waiting for recovery bed
  std::mt19937 rng_;
  std::exponential_distribution<double> urgent_interarrival_;
  double horizon_minutes_ = 0.0;
  double operating_room_busy_minutes_ = 0.0;
  double surgeon_busy_minutes_ = 0.0;

  double recovery_busy_minutes_ = 0.0;
  int busy_operating_rooms_ = 0;
  int busy_recovery_beds_ = 0;
  int busy_surgeons_ = 0;
};

std::string scheduling_policy_to_string(SchedulingPolicy policy);
