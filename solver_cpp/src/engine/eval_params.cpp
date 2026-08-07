// ============================================================================
//  Registre des parametres : permet de lire/ecrire n'importe quel poids par son
//  nom (CLI, fichier, tuning automatique) sans toucher au code.
// ============================================================================
#include "eval_params.hpp"
#include "eval_registry.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace tzaar {

namespace {
struct Field { const char* name; size_t off; bool isBool; };

#define F(member) { #member, offsetof(EvalParams, member), false }
#define B(member) { #member, offsetof(EvalParams, member), true  }

const Field FIELDS[] = {
    { "typeValue0", offsetof(EvalParams, typeValue) + 0 * sizeof(int), false },
    { "typeValue1", offsetof(EvalParams, typeValue) + 1 * sizeof(int), false },
    { "typeValue2", offsetof(EvalParams, typeValue) + 2 * sizeof(int), false },
    F(countCurveTail), F(heightWeight), F(pieceWeight),
    F(raceThreshold), F(raceTempo), F(raceEdge),
    F(mobilityWeight), F(threatWeight), F(zugzwangWeight),
    F(lazyMargin), F(aspirationWindow), F(lmrMinDepth), F(lmrMinMove),
    F(lmpBase), F(lmpFactor), F(capBase), F(capFactor),
    B(useDynamic), B(useLMR), B(useTT), B(useLMP),
};
#undef F
#undef B
constexpr int NFIELDS = (int)(sizeof(FIELDS) / sizeof(FIELDS[0]));
}

bool set_param(EvalParams& p, const std::string& name, int value) {
    // Cas special : countCurve[i]
    if (name.rfind("countCurve", 0) == 0 && name.size() > 10 && name[10] >= '0' && name[10] <= '9') {
        int i = std::atoi(name.c_str() + 10);
        if (i < 0 || i > 15) return false;
        p.countCurve[i] = value;
        return true;
    }
    char* base = reinterpret_cast<char*>(&p);
    for (int i = 0; i < NFIELDS; ++i) {
        if (name == FIELDS[i].name) {
            if (FIELDS[i].isBool) *reinterpret_cast<bool*>(base + FIELDS[i].off) = value != 0;
            else                  *reinterpret_cast<int*>(base + FIELDS[i].off)  = value;
            return true;
        }
    }
    return false;
}

std::string dump_params(const EvalParams& p) {
    std::string out;
    char buf[128];
    const char* base = reinterpret_cast<const char*>(&p);
    for (int i = 0; i < NFIELDS; ++i) {
        int v = FIELDS[i].isBool ? (int)*reinterpret_cast<const bool*>(base + FIELDS[i].off)
                                 : *reinterpret_cast<const int*>(base + FIELDS[i].off);
        std::snprintf(buf, sizeof buf, "%s=%d ", FIELDS[i].name, v);
        out += buf;
    }
    for (int i = 1; i <= 15; ++i) {
        std::snprintf(buf, sizeof buf, "countCurve%d=%d ", i, p.countCurve[i]);
        out += buf;
    }
    return out;
}

std::vector<std::string> param_names() {
    std::vector<std::string> v;
    for (int i = 0; i < NFIELDS; ++i) v.push_back(FIELDS[i].name);
    for (int i = 1; i <= 15; ++i) v.push_back("countCurve" + std::to_string(i));
    return v;
}

} // namespace tzaar
