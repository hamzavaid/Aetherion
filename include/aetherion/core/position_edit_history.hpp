#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::core {

enum class PositionTargetKind { body, probe };

struct PositionTarget {
    PositionTargetKind kind{};
    std::uint64_t id{};
    [[nodiscard]] bool operator==(const PositionTarget&) const = default;
};

/// Bounded per-selection history of explicit world-frame position edits in meters. It excludes
/// physics motion and other properties. Each key repeat is one reversible edit; `reset` returns
/// to the position at selection/rebase and is itself undoable. No scene is mutated here.
class PositionEditHistory final {
  public:
    explicit PositionEditHistory(std::size_t capacity = 256);
    /// Returns true when a new body/probe target replaces the previous history.
    [[nodiscard]] bool select(PositionTarget target, const math::Vec3d& position_m);
    void rebase(const math::Vec3d& position_m);
    void clear() noexcept;
    [[nodiscard]] bool nudge(const math::Vec3d& delta_m);
    [[nodiscard]] bool undo();
    [[nodiscard]] bool reset();
    [[nodiscard]] bool canUndo() const noexcept { return !undo_positions_m_.empty(); }
    [[nodiscard]] std::optional<PositionTarget> target() const noexcept { return target_; }
    [[nodiscard]] std::optional<math::Vec3d> current() const noexcept { return current_m_; }

  private:
    void rememberCurrent();

    std::size_t capacity_;
    std::optional<PositionTarget> target_;
    std::optional<math::Vec3d> initial_m_;
    std::optional<math::Vec3d> current_m_;
    std::vector<math::Vec3d> undo_positions_m_;
};

} // namespace aetherion::core
