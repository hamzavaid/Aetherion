#pragma once

#include <string>

#include "aetherion/core/command_queue.hpp"

namespace aetherion::core {

/// Creates a non-overlapping interactive body using the active runtime's SI physics context.
/// Electrostatic scenes receive a +1 microcoulomb test charge; magnetic-only scenes receive a
/// +1 coulomb test particle. Mechanical scenes remain neutral.
[[nodiscard]] Body makeInteractiveBody(const Scene& scene, const RuntimeSettings& runtime,
                                       std::string name);

} // namespace aetherion::core
