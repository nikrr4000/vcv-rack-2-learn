// Builds the checker's patch model from the running engine (UI thread only).
#pragma once
#include <set>

#include "atlas.hpp"
#include "check/Checker.hpp"

namespace atlas {

/** Modules in `ids` (all modules when empty) with param display values and port names, and the cables between them. */
check::Patch rackPatch(const std::set<int64_t>& ids);

/** One output voltage of a module (0 when missing). */
float outputVoltage(int64_t moduleId, int outputId, bool* ok = nullptr);

} // namespace atlas
