/** @file
 * Scope-bound rollback of body kinematics when a numerical operation cannot be accepted.
 */
#pragma once

#include <algorithm>
#include <vector>

#include "aetherion/core/scene.hpp"

namespace aetherion::core {

/// Restores every body's SI state unless an integration/evaluation finishes successfully.
/// Force callbacks may edit acceleration, but must preserve body identity and scene topology.
class StateTransaction final {
  public:
    explicit StateTransaction(Scene& scene) : scene_(scene) {
        initial_.reserve(scene.size());
        for (const auto& body : scene.bodies())
            initial_.push_back(body.state);
    }
    ~StateTransaction() {
        if (!committed_) {
            for (std::size_t index = 0; index < std::min(initial_.size(), scene_.size()); ++index)
                scene_.bodies()[index].state = initial_[index];
        }
    }
    StateTransaction(const StateTransaction&) = delete;
    StateTransaction& operator=(const StateTransaction&) = delete;
    /// Accepts the proposed body state; unsuccessful return paths roll back automatically.
    void commit() noexcept { committed_ = true; }

  private:
    Scene& scene_;
    std::vector<BodyState> initial_;
    bool committed_{};
};

} // namespace aetherion::core
