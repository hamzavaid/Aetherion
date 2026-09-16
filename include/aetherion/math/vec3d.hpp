#pragma once

#include <cstddef>

namespace aetherion::math {

/// Double-precision Cartesian vector used by physics. Components carry the units of their field.
struct Vec3d {
    double x{};
    double y{};
    double z{};

    [[nodiscard]] constexpr Vec3d operator+() const noexcept { return *this; }
    [[nodiscard]] constexpr Vec3d operator-() const noexcept { return {-x, -y, -z}; }
    constexpr Vec3d& operator+=(const Vec3d& rhs) noexcept;
    constexpr Vec3d& operator-=(const Vec3d& rhs) noexcept;
    constexpr Vec3d& operator*=(double scalar) noexcept;
    Vec3d& operator/=(double scalar);

    [[nodiscard]] double squaredNorm() const noexcept;
    [[nodiscard]] double norm() const noexcept;
    [[nodiscard]] bool isFinite() const noexcept;
    [[nodiscard]] Vec3d normalized(double minimum_norm = 1.0e-15) const;
    [[nodiscard]] constexpr bool operator==(const Vec3d&) const noexcept = default;
};

[[nodiscard]] constexpr Vec3d operator+(Vec3d lhs, const Vec3d& rhs) noexcept;
[[nodiscard]] constexpr Vec3d operator-(Vec3d lhs, const Vec3d& rhs) noexcept;
[[nodiscard]] constexpr Vec3d operator*(Vec3d vector, double scalar) noexcept;
[[nodiscard]] constexpr Vec3d operator*(double scalar, Vec3d vector) noexcept;
[[nodiscard]] Vec3d operator/(Vec3d vector, double scalar);
[[nodiscard]] constexpr double dot(const Vec3d& lhs, const Vec3d& rhs) noexcept;
[[nodiscard]] constexpr Vec3d cross(const Vec3d& lhs, const Vec3d& rhs) noexcept;

constexpr Vec3d& Vec3d::operator+=(const Vec3d& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}
constexpr Vec3d& Vec3d::operator-=(const Vec3d& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}
constexpr Vec3d& Vec3d::operator*=(double scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}
constexpr Vec3d operator+(Vec3d lhs, const Vec3d& rhs) noexcept { return lhs += rhs; }
constexpr Vec3d operator-(Vec3d lhs, const Vec3d& rhs) noexcept { return lhs -= rhs; }
constexpr Vec3d operator*(Vec3d vector, double scalar) noexcept { return vector *= scalar; }
constexpr Vec3d operator*(double scalar, Vec3d vector) noexcept { return vector *= scalar; }
constexpr double dot(const Vec3d& lhs, const Vec3d& rhs) noexcept {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
constexpr Vec3d cross(const Vec3d& lhs, const Vec3d& rhs) noexcept {
    return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

} // namespace aetherion::math
