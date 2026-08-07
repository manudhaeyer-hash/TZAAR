// ============================================================================
//  Protocole CodinGame. Isole ici pour que le moteur reste testable hors I/O.
//
//   init  : myId
//   tour  : actionPhase / validPieceCount / (x y owner type height) * n
//   sortie: "MOVE x1 y1 x2 y2" ou "WAIT"
// ============================================================================
#ifndef TZAAR_IO_PROTOCOL_HPP
#define TZAAR_IO_PROTOCOL_HPP

#include "../core/position.hpp"
#include <istream>

namespace tzaar {

struct TurnInput {
    int phase = PHASE_CAPTURE;
    Position pos;
    bool ok = false;
};

// Lit un tour complet. Renvoie ok=false sur EOF (fin de partie).
TurnInput read_turn(std::istream& in);

} // namespace tzaar
#endif
