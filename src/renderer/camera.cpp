#include "aetherion/renderer/camera.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace aetherion::renderer {
namespace {

constexpr double half_pi = 1.5707963267948966;

math::Vec3d forward(const Camera& camera) {
    return (camera.targetWorld() - camera.positionWorld()).normalized();
}

Mat4f multiply(const Mat4f& lhs, const Mat4f& rhs) {
    Mat4f result{};
    for (std::size_t column = 0; column < 4U; ++column) {
        for (std::size_t row = 0; row < 4U; ++row) {
            for (std::size_t inner = 0; inner < 4U; ++inner) {
                result[column * 4U + row] += lhs[inner * 4U + row] * rhs[column * 4U + inner];
            }
        }
    }
    return result;
}

} // namespace

void Camera::orbit(double yaw_delta_rad, double pitch_delta_rad) noexcept {
    if (!std::isfinite(yaw_delta_rad) || !std::isfinite(pitch_delta_rad)) {
        return;
    }
    yaw_rad_ = std::remainder(yaw_rad_ + yaw_delta_rad, 2.0 * std::numbers::pi);
    pitch_rad_ = std::clamp(pitch_rad_ + pitch_delta_rad, -half_pi + 1.0e-4, half_pi - 1.0e-4);
}

void Camera::pan(double horizontal_m, double vertical_m) noexcept {
    if (!std::isfinite(horizontal_m) || !std::isfinite(vertical_m)) {
        return;
    }
    const auto direction = forward(*this);
    const auto right = math::cross(direction, {0.0, 1.0, 0.0}).normalized();
    const auto up = math::cross(right, direction).normalized();
    target_world_m_ += horizontal_m * right + vertical_m * up;
}

void Camera::zoom(double logarithmic_delta) noexcept {
    if (!std::isfinite(logarithmic_delta)) {
        return;
    }
    const double exponent = std::clamp(logarithmic_delta * 0.1, -10.0, 10.0);
    distance_m_ = std::clamp(distance_m_ * std::exp(exponent), 1.0e-6, 1.0e20);
}

void Camera::focus(const math::Vec3d& target_world_m, double bounding_radius_m) {
    if (!target_world_m.isFinite() || !std::isfinite(bounding_radius_m) ||
        bounding_radius_m <= 0.0) {
        throw std::invalid_argument("camera focus requires finite target and positive radius in m");
    }
    target_world_m_ = target_world_m;
    distance_m_ = std::max(2.5 * bounding_radius_m, 1.0e-6);
}

void Camera::reset() noexcept {
    target_world_m_ = {};
    distance_m_ = 10.0;
    yaw_rad_ = 0.0;
    pitch_rad_ = 0.0;
}

math::Vec3d Camera::positionWorld() const noexcept {
    const double horizontal = distance_m_ * std::cos(pitch_rad_);
    return target_world_m_ + math::Vec3d{horizontal * std::sin(yaw_rad_),
                                         distance_m_ * std::sin(pitch_rad_),
                                         horizontal * std::cos(yaw_rad_)};
}

Ray Camera::rayFromNdc(double x_ndc, double y_ndc, double aspect_ratio) const {
    if (!std::isfinite(x_ndc) || !std::isfinite(y_ndc) || !std::isfinite(aspect_ratio) ||
        aspect_ratio <= 0.0) {
        throw std::invalid_argument("camera ray requires finite NDC and positive aspect ratio");
    }
    const auto direction = forward(*this);
    const auto right = math::cross(direction, {0.0, 1.0, 0.0}).normalized();
    const auto up = math::cross(right, direction).normalized();
    const double tangent = std::tan(vertical_fov_rad_ * 0.5);
    return {.origin = positionWorld(),
            .direction =
                (direction + right * (x_ndc * aspect_ratio * tangent) + up * (y_ndc * tangent))
                    .normalized()};
}

Mat4f Camera::viewProjection(double aspect_ratio, double meters_to_render_units) const {
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0.0) {
        throw std::invalid_argument("camera projection requires a positive finite aspect ratio");
    }
    if (!std::isfinite(meters_to_render_units) || meters_to_render_units <= 0.0) {
        throw std::invalid_argument("camera projection requires a positive finite render scale");
    }
    const auto direction = forward(*this);
    const auto right = math::cross(direction, {0.0, 1.0, 0.0}).normalized();
    const auto up = math::cross(right, direction).normalized();
    const Mat4f view = {static_cast<float>(right.x),
                        static_cast<float>(up.x),
                        static_cast<float>(-direction.x),
                        0.0F,
                        static_cast<float>(right.y),
                        static_cast<float>(up.y),
                        static_cast<float>(-direction.y),
                        0.0F,
                        static_cast<float>(right.z),
                        static_cast<float>(up.z),
                        static_cast<float>(-direction.z),
                        0.0F,
                        0.0F,
                        0.0F,
                        0.0F,
                        1.0F};
    const double rendered_distance = distance_m_ * meters_to_render_units;
    const double near_plane = std::max(rendered_distance * 1.0e-6, 1.0e-6);
    const double far_plane = std::max(rendered_distance * 1.0e6, near_plane * 10.0);
    const double scale = 1.0 / std::tan(vertical_fov_rad_ * 0.5);
    Mat4f projection{};
    projection[0] = static_cast<float>(scale / aspect_ratio);
    projection[5] = static_cast<float>(scale);
    projection[10] = static_cast<float>((far_plane + near_plane) / (near_plane - far_plane));
    projection[11] = -1.0F;
    projection[14] = static_cast<float>((2.0 * far_plane * near_plane) / (near_plane - far_plane));
    return multiply(projection, view);
}

} // namespace aetherion::renderer
