#ifndef TZAAR_ENGINE_EVAL_REGISTRY_HPP
#define TZAAR_ENGINE_EVAL_REGISTRY_HPP

#include "eval_params.hpp"
#include <string>
#include <vector>
#include <cstdlib>

namespace tzaar {

// Ecrit un parametre par son nom. Renvoie false si le nom est inconnu.
bool set_param(EvalParams& p, const std::string& name, int value);
// Serialise tous les parametres sous forme "nom=valeur ...".
std::string dump_params(const EvalParams& p);
// Liste des noms modifiables (utile pour un tuner automatique).
std::vector<std::string> param_names();

} // namespace tzaar
#endif
