#include "core/patient.h"

Patient::Patient(int id, PatientType type, double arrival)
    : id(id), type(type), arrival_time(arrival) 
{
    // Les autres valeurs restent à leurs défauts (-1.0 ou 0.0)
}

// Exemple de méthode simple pour vérifier l'urgence
bool Patient::is_urgent() const {
    return type == PatientType::Urgent;
}

// Vérifie si le patient a fini son parcours (sortie de réveil)
bool Patient::is_completed() const {
    return end_recovery_time >= 0.0;
}