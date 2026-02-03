#include "core/simulation.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

std::string format_minutes(double minutes) {
  std::ostringstream os;
  os << std::fixed << std::setprecision(1) << minutes;
  return os.str();
}

} // namespace

std::string scheduling_policy_to_string(SchedulingPolicy policy) {
  switch (policy) {
  case SchedulingPolicy::Fifo:
    return "fifo";
  case SchedulingPolicy::PriorityFirst:
    return "priorite";
  case SchedulingPolicy::Balanced:
    return "equilibre";
  }
  return "unknown";
}

Simulation::Simulation(SimulationConfig config)
    : config_(std::move(config)),
      event_compare_(
          [](const Event &a, const Event &b) { return a.time > b.time; }),
      events_(event_compare_), rng_(config_.seed),
      urgent_interarrival_((config_.urgent_rate_per_hour > 0.0)
                               ? (config_.urgent_rate_per_hour / 60.0)
                               : 1.0) {
  horizon_minutes_ = config_.horizon_hours * 60.0;
}

double Simulation::draw_positive_duration(double mean_minutes) {
  const double stddev = std::max(1.0, mean_minutes * 0.25);
  std::normal_distribution<double> dist(mean_minutes, stddev);
  double value = -1.0;
  int guard = 0;
  while (value <= 1.0 && guard < 16) {
    value = dist(rng_);
    ++guard;
  }
  return (value <= 1.0) ? mean_minutes : value;
}

double Simulation::draw_urgent_interarrival_minutes() {
  // urgent_interarrival_ uses rate per minute
  const double sample = urgent_interarrival_(rng_);
  return (sample <= 0.0) ? 1.0 : sample;
}

void Simulation::set_log_sink(
    std::function<void(const std::string &, double)> sink) {
  log_sink_ = std::move(sink);
}

void Simulation::log_event(const std::string &message, double time) {
  if (!config_.trace_events)
    return;
  if (log_sink_) {
    log_sink_(message, time);
  } else {
    std::cout << "[t=" << format_minutes(time) << " min] " << message << '\n';
  }
}

void Simulation::seed_patients() {
  patients_.clear();
  waiting_patients_.clear();
  recovery_waiting_.clear();
  events_ = EventQueue(event_compare_);
  busy_operating_rooms_ = 0;
  busy_surgeons_ = 0;

  busy_recovery_beds_ = 0;
  operating_room_busy_minutes_ = 0.0;
  surgeon_busy_minutes_ = 0.0;
  recovery_busy_minutes_ = 0.0;

  const double elective_window_minutes =
      std::max(0.1, config_.elective_window_hours) * 60.0;

  for (int i = 0; i < config_.elective_patients; ++i) {
    const double position = (i + 0.5) / std::max(1, config_.elective_patients);
    const double arrival = position * elective_window_minutes;

    Patient p(static_cast<int>(patients_.size()), PatientType::Elective,
              arrival);
    p.surgery_duration =
        draw_positive_duration(config_.mean_surgery_minutes_elective);
    p.recovery_duration = draw_positive_duration(config_.mean_recovery_minutes);
    patients_.push_back(p);

    push_event(Event{arrival, EventType::Arrival, p.id});
  }

  if (config_.urgent_rate_per_hour > 0.0) {
    double current = draw_urgent_interarrival_minutes();

    while (current <= horizon_minutes_) {
      Patient p(static_cast<int>(patients_.size()), PatientType::Urgent,
                current);

      p.surgery_duration =
          draw_positive_duration(config_.mean_surgery_minutes_urgent);

      p.recovery_duration =
          draw_positive_duration(config_.mean_recovery_minutes);

      patients_.push_back(p);

      push_event(Event{current, EventType::Arrival, p.id});

      current += draw_urgent_interarrival_minutes();
    }
  }
}

void Simulation::push_event(const Event &event) { events_.push(event); }

int Simulation::pick_next_patient(double now) {
  if (waiting_patients_.empty())
    return -1;
  int best_index = 0;
  auto better = [&](int lhs_idx, int rhs_idx) {
    const Patient &a = patients_[waiting_patients_[lhs_idx]];
    const Patient &b = patients_[waiting_patients_[rhs_idx]];
    switch (config_.policy) {
    case SchedulingPolicy::Fifo: {
      return a.arrival_time < b.arrival_time;
    }
    case SchedulingPolicy::PriorityFirst: {
      const int priority_a = (a.type == PatientType::Urgent) ? 1 : 0;
      const int priority_b = (b.type == PatientType::Urgent) ? 1 : 0;
      if (priority_a != priority_b)
        return priority_a > priority_b;
      return a.arrival_time < b.arrival_time;
    }
    case SchedulingPolicy::Balanced: {
      const double wait_a = now - a.arrival_time;
      const double wait_b = now - b.arrival_time;
      const double score_a =
          ((a.type == PatientType::Urgent) ? 2.0 : 1.0) + wait_a / 60.0;
      const double score_b =
          ((b.type == PatientType::Urgent) ? 2.0 : 1.0) + wait_b / 60.0;
      if (std::abs(score_a - score_b) > 1e-6)
        return score_a > score_b;
      return a.arrival_time < b.arrival_time;
    }
    }
    return false;
  };

  for (size_t i = 1; i < waiting_patients_.size(); ++i) {
    if (better(static_cast<int>(i), best_index)) {
      best_index = static_cast<int>(i);
    }
  }
  const int patient_id = waiting_patients_[best_index];
  waiting_patients_[best_index] = waiting_patients_.back();
  waiting_patients_.pop_back();
  return patient_id;
}

void Simulation::try_schedule_surgery(double now) {
<<<<<<< HEAD
=======
  // On ne commence plus de nouvelles chirurgies si l'horizon est atteint ou
  // dépassé.
  if (now >= horizon_minutes_) {
    return;
  }

>>>>>>> 45492c1 (Fix git de merde)
  while (busy_operating_rooms_ < config_.operating_rooms &&
         busy_surgeons_ < config_.surgeon_count && !waiting_patients_.empty()) {
    const int patient_id = pick_next_patient(now);
    if (patient_id < 0)
      return;
    Patient &p = patients_[patient_id];
    p.start_surgery_time = now;
    p.end_surgery_time = now + p.surgery_duration;
    ++busy_operating_rooms_;
    ++busy_surgeons_;
    operating_room_busy_minutes_ += p.surgery_duration;
    surgeon_busy_minutes_ += p.surgery_duration;
    push_event(Event{p.end_surgery_time, EventType::SurgeryEnd, p.id});
    log_event(
        "Debut chirurgie patient " + std::to_string(p.id) +
            ". (Chirurgien occupe : " + std::to_string(busy_surgeons_) + "/" +
            std::to_string(config_.surgeon_count) + ")" +
            (p.type == PatientType::Urgent ? " (urgence)" : " (programme)"),
        now);
  }
}

void Simulation::try_start_recovery(double now) {
  while (busy_recovery_beds_ < config_.recovery_beds &&
         !recovery_waiting_.empty()) {
    const int patient_id = recovery_waiting_.front();
    recovery_waiting_.pop_front();
    Patient &p = patients_[patient_id];
    p.start_recovery_time = now;
    p.end_recovery_time = now + p.recovery_duration;
    ++busy_recovery_beds_;
    recovery_busy_minutes_ += p.recovery_duration;
    push_event(Event{p.end_recovery_time, EventType::RecoveryEnd, p.id});
    log_event("Debut reveil patient " + std::to_string(p.id), now);
  }
}

void Simulation::handle_arrival(const Event &event, double now) {
  Patient &p = patients_[event.patient_id];
  waiting_patients_.push_back(p.id);
  log_event("Patient " + std::to_string(p.id) +
                (p.type == PatientType::Urgent ? " arrive (urgence)"
                                               : " arrive (programme)"),
            now);
  try_schedule_surgery(now);
}

void Simulation::handle_surgery_end(const Event &event, double now) {
  Patient &p = patients_[event.patient_id];
  if (busy_operating_rooms_ > 0) {
    --busy_operating_rooms_;
  }
  if (busy_surgeons_ > 0) {
    --busy_surgeons_;
  }

  log_event("Fin chirurgie patient " + std::to_string(p.id) +
                ". Chirurgien libere" + ". Debut nettoyage (" +
                format_minutes(config_.cleaning_time_minutes) + " min)",
            now);
  double cleaning_end_time = now + config_.cleaning_time_minutes;
  operating_room_busy_minutes_ += config_.cleaning_time_minutes;

  push_event(Event{cleaning_end_time, EventType::CleaningEnd, p.id});

  recovery_waiting_.push_back(p.id);
  try_start_recovery(now);

  try_schedule_surgery(now);
}

void Simulation::handle_cleaning_end(const Event &event, double now) {
  // Le patient ID sert juste de référence ici, il est déjà en réveil.

  // Maintenant la salle est propre et libre
  if (busy_operating_rooms_ > 0) {
    --busy_operating_rooms_;
  }

  log_event("Nettoyage termine apres patient " +
                std::to_string(event.patient_id) + ". Salle libre.",
            now);

  // C'est SEULEMENT maintenant qu'on peut prendre un nouveau patient
  try_schedule_surgery(now);
}

void Simulation::handle_recovery_end(const Event &event, double now) {
  Patient &p = patients_[event.patient_id];
  if (busy_recovery_beds_ > 0) {
    --busy_recovery_beds_;
  }
  log_event("Fin reveil patient " + std::to_string(p.id), now);
  (void)p;
  try_start_recovery(now);
}

SimulationReport Simulation::run() {
  seed_patients();

  int patients_arrived = 0;
  int urgent_arrived = 0;
  int elective_arrived = 0;

  while (!events_.empty()) {
    const Event current = events_.top();
    events_.pop();
    const double now = current.time;
    if (now > horizon_minutes_ * 1.5) {
      // Guard against runaway scheduling; horizon governs arrivals.
      break;
    }
    switch (current.type) {
    case EventType::Arrival:
      ++patients_arrived;
      if (patients_[current.patient_id].type == PatientType::Urgent) {
        ++urgent_arrived;
      } else {
        ++elective_arrived;
      }
      handle_arrival(current, now);
      break;
    case EventType::CleaningEnd:
      handle_cleaning_end(current, now);
      break;
    case EventType::SurgeryEnd:
      handle_surgery_end(current, now);
      break;
    case EventType::RecoveryEnd:
      handle_recovery_end(current, now);
      break;
    }
  }

  // Final scheduling if some patients remained waiting without events.
  // This should be rare but keeps counters consistent.
  double now = horizon_minutes_;
  try_schedule_surgery(now);
  try_start_recovery(now);

  SimulationReport report;
  report.patients_arrived = patients_arrived;
  report.urgent_arrived = urgent_arrived;
  report.elective_arrived = elective_arrived;

  double total_wait_to_surgery = 0.0;
  double total_wait_to_recovery = 0.0;
  double total_system_time = 0.0;
  int patients_operated = 0;
  int patients_recovered = 0;
  int patients_started_recovery = 0;

  for (const Patient &p : patients_) {
    if (p.start_surgery_time >= 0.0) {
      ++patients_operated;
      const double wait = p.start_surgery_time - p.arrival_time;
      total_wait_to_surgery += wait;
      report.max_wait_to_surgery = std::max(report.max_wait_to_surgery, wait);

      // --- NOUVEAU : Calcul du Retard ---
      // Si c'est un patient programmé et qu'il attend > 15 min, il est en
      // retard
<<<<<<< HEAD
      if (p.type == PatientType::Elective && wait > 15.0) {
=======
      // if (p.type == PatientType::Elective && wait > 15.0) {
      if (wait > 15.0) {
>>>>>>> 45492c1 (Fix git de merde)
        report.operations_delayed++;
      }
    }
    if (p.start_recovery_time >= 0.0) {
      ++patients_started_recovery;
      total_wait_to_recovery += p.start_recovery_time - p.end_surgery_time;
    }
    if (p.end_recovery_time >= 0.0) {
      ++patients_recovered;
      total_system_time += p.end_recovery_time - p.arrival_time;
    }
  }

  report.patients_operated = patients_operated;
  report.patients_completed = patients_recovered;
  // --- NOUVEAU : Calcul des Annulations ---
  // Les patients "Annulés" sont ceux qui restent en attente (pending_waiting)
  report.pending_waiting = std::max(0, patients_arrived - patients_operated);
  report.operations_cancelled = report.pending_waiting;

  if (patients_operated > 0) {
    report.average_wait_to_surgery = total_wait_to_surgery / patients_operated;
  }
  if (patients_recovered > 0) {
    report.average_total_time_in_system =
        total_system_time / patients_recovered;
  }
  if (patients_started_recovery > 0) {
    report.average_wait_to_recovery =
        total_wait_to_recovery / patients_started_recovery;
  }

  const double denominator_or = horizon_minutes_ * config_.operating_rooms;
  const double denominator_recovery =
      horizon_minutes_ * std::max(1, config_.recovery_beds);
  if (denominator_or > 0.0) {
    report.operating_room_utilization =
        std::min(1.0, operating_room_busy_minutes_ / denominator_or);
  }
  if (denominator_recovery > 0.0) {
    report.recovery_bed_utilization =
        std::min(1.0, recovery_busy_minutes_ / denominator_recovery);
  }
  if (config_.horizon_hours > 0.0) {
    report.throughput_per_hour =
        static_cast<double>(patients_recovered) / config_.horizon_hours;
  }
  const double denominator_surgeon =
      horizon_minutes_ * std::max(1, config_.surgeon_count);
  if (denominator_surgeon > 0.0) {
    report.surgeon_utilization =
        std::min(1.0, surgeon_busy_minutes_ / denominator_surgeon);
  }

  return report;
}
