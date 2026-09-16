#pragma once

#include "aetherion/math/vec3d.hpp"
#include "aetherion/renderer/types.hpp"

namespace aetherion::renderer {

/// Double-precision orbit camera. Angles are radians and all distances are physical meters.
class Camera final {
  public:
    Camera() = default;

    void orbit(double yaw_delta_rad, double pitch_delta_rad) noexcept;
    void pan(double horizontal_m, double vertical_m) noexcept;
    void zoom(double logarithmic_delta) noexcept;
    void focus(const math::Vec3d& target_world_m, double bounding_radius_m);
    void reset() noexcept;

    [[nodiscard]] math::Vec3d positionWorld() const noexcept;
    [[nodiscard]] const math::Vec3d& targetWorld() const noexcept { return target_world_m_; }
    [[nodiscard]] double distanceMeters() const noexcept { return distance_m_; }
    [[nodiscard]] Ray rayFromNdc(double x_ndc, double y_ndc, double aspect_ratio) const;
    [[nodiscard]] Mat4f viewProjection(double aspect_ratio,
                                       double meters_to_render_units = 1.0) const;

  private:
    math::Vec3d target_world_m_{};
    double distance_m_{10.0};
    double yaw_rad_{0.65};
    double pitch_rad_{0.65};
    double vertical_fov_rad_{0.7853981633974483};
};

} // namespace aetherion::renderer
