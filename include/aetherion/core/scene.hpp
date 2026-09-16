#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "aetherion/core/error.hpp"
#include "aetherion/math/vec3d.hpp"

namespace aetherion::core {

using EntityId = std::uint64_t;

enum class Interaction : std::uint32_t { gravity = 1U << 0U };

struct InteractionMask {
    std::uint32_t bits{static_cast<std::uint32_t>(Interaction::gravity)};

    [[nodiscard]] constexpr bool contains(Interaction interaction) const noexcept {
        return (bits & static_cast<std::uint32_t>(interaction)) != 0U;
    }
    constexpr void set(Interaction interaction, bool enabled) noexcept {
        const auto flag = static_cast<std::uint32_t>(interaction);
        bits = enabled ? bits | flag : bits & ~flag;
    }
    [[nodiscard]] constexpr bool operator==(const InteractionMask&) const noexcept = default;
};

/// SI-valued dynamical state in the inertial world reference frame.
struct BodyState {
    math::Vec3d position_m;
    math::Vec3d velocity_mps;
    math::Vec3d acceleration_mps2;
    [[nodiscard]] constexpr bool operator==(const BodyState&) const noexcept = default;
};

/// Massive spherical entity. Render appearance is deliberately not part of physical state.
struct Body {
    EntityId id{};
    std::string name;
    double mass_kg{};
    double charge_C{};
    double radius_m{};
    bool fixed{};
    InteractionMask interactions;
    BodyState state;
};

/// Deterministically ordered owner of bodies and monotonically allocated entity IDs.
class Scene final {
  public:
    [[nodiscard]] Result<EntityId> createBody(Body body);
    [[nodiscard]] bool remove(EntityId id) noexcept;
    [[nodiscard]] Body* find(EntityId id) noexcept;
    [[nodiscard]] const Body* find(EntityId id) const noexcept;
    [[nodiscard]] std::vector<Body>& bodies() noexcept { return bodies_; }
    [[nodiscard]] const std::vector<Body>& bodies() const noexcept { return bodies_; }
    [[nodiscard]] std::size_t size() const noexcept { return bodies_.size(); }
    [[nodiscard]] static Status validate(const Body& body);

  private:
    std::vector<Body> bodies_;
    EntityId next_id_{1};
};

} // namespace aetherion::core
