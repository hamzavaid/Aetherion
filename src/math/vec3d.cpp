/** @file
 * Double-precision Cartesian arithmetic; field names supply units and normalization returns a unit
 * vector.
 */
#include "aetherion/math/vec3d.hpp"

#include <cmath>
#include <stdexcept>

namespace aetherion::math {

Vec3d& Vec3d::operator/=(double scalar) {
    if (scalar == 0.0 || !std::isfinite(scalar)) {
        throw std::domain_error("Vec3d division requires a finite non-zero scalar");
    }
    // Divide components directly: forming 1/scalar can overflow for a valid subnormal divisor.
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

double Vec3d::squaredNorm() const noexcept { return dot(*this, *this); }
double Vec3d::norm() const noexcept { return std::hypot(x, y, z); }
bool Vec3d::isFinite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

Vec3d Vec3d::normalized(double minimum_norm) const {
    const double magnitude = norm();
    if (!std::isfinite(minimum_norm) || minimum_norm <= 0.0 || !std::isfinite(magnitude) ||
        magnitude < minimum_norm) {
        throw std::domain_error("Vec3d cannot normalize below the positive finite guard norm");
    }
    return *this / magnitude;
}

Vec3d operator/(Vec3d vector, double scalar) { return vector /= scalar; }

} // namespace aetherion::math
