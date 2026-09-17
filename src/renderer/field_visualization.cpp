#include "aetherion/renderer/field_visualization.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace aetherion::renderer {
namespace {

math::Vec3d selectedVector(const physics::fields::FieldSample& sample, ObservedField field) {
    return field == ObservedField::electric ? sample.electric_Vpm : sample.magnetic_T;
}

bool insideRegion(const math::Vec3d& point, const FieldRegion& region) {
    const auto offset = point - region.center_m;
    return std::abs(offset.x) <= region.half_extent_m.x &&
           std::abs(offset.y) <= region.half_extent_m.y &&
           std::abs(offset.z) <= region.half_extent_m.z;
}

void validate(const FieldVisualizationSettings& settings) {
    if (!settings.region.center_m.isFinite() || !settings.region.half_extent_m.isFinite() ||
        settings.region.half_extent_m.x <= 0.0 || settings.region.half_extent_m.y <= 0.0 ||
        settings.region.half_extent_m.z <= 0.0 || settings.vectors.resolution < 2U ||
        settings.vectors.resolution > 32U || !std::isfinite(settings.vectors.visual_length_m) ||
        settings.vectors.visual_length_m <= 0.0 ||
        !std::isfinite(settings.vectors.reference_magnitude) ||
        settings.vectors.reference_magnitude <= 0.0 ||
        !std::isfinite(settings.vectors.minimum_magnitude) ||
        settings.vectors.minimum_magnitude < 0.0 ||
        !std::isfinite(settings.vectors.maximum_magnitude) ||
        settings.vectors.maximum_magnitude < settings.vectors.minimum_magnitude ||
        !std::isfinite(settings.lines.step_size_m) || settings.lines.step_size_m <= 0.0 ||
        settings.lines.maximum_steps == 0U || settings.lines.maximum_total_steps == 0U ||
        !std::isfinite(settings.lines.maximum_length_m) || settings.lines.maximum_length_m <= 0.0 ||
        !std::isfinite(settings.lines.minimum_field_magnitude) ||
        settings.lines.minimum_field_magnitude < 0.0) {
        throw std::invalid_argument("field visualization settings are outside finite safe bounds");
    }
}

bool unitField(const physics::fields::IFieldProvider& provider, ObservedField field,
               const math::Vec3d& position_m, double time_s, double minimum_magnitude,
               math::Vec3d& direction) {
    const auto sample = provider.sample(position_m, time_s);
    if (!sample.valid)
        return false;
    const auto vector = selectedVector(sample, field);
    const double magnitude = vector.norm();
    if (!std::isfinite(magnitude) || magnitude <= minimum_magnitude)
        return false;
    direction = vector / magnitude;
    return direction.isFinite();
}

std::vector<math::Vec3d> traceDirection(const physics::fields::IFieldProvider& provider,
                                        const FieldVisualizationSettings& settings,
                                        const math::Vec3d& seed_m, double time_s, double sign,
                                        std::size_t& remaining_work) {
    std::vector<math::Vec3d> points;
    points.push_back(seed_m);
    auto position = seed_m;
    double length_m = 0.0;
    for (std::size_t step = 0; step < settings.lines.maximum_steps && remaining_work > 0U; ++step) {
        --remaining_work;
        math::Vec3d k1;
        math::Vec3d k2;
        math::Vec3d k3;
        math::Vec3d k4;
        const double h = sign * settings.lines.step_size_m;
        if (!unitField(provider, settings.field, position, time_s,
                       settings.lines.minimum_field_magnitude, k1) ||
            !unitField(provider, settings.field, position + k1 * (0.5 * h), time_s,
                       settings.lines.minimum_field_magnitude, k2) ||
            !unitField(provider, settings.field, position + k2 * (0.5 * h), time_s,
                       settings.lines.minimum_field_magnitude, k3) ||
            !unitField(provider, settings.field, position + k3 * h, time_s,
                       settings.lines.minimum_field_magnitude, k4)) {
            break;
        }
        const auto next = position + (k1 + 2.0 * k2 + 2.0 * k3 + k4) * (h / 6.0);
        if (!next.isFinite() || !insideRegion(next, settings.region))
            break;
        const double segment_length = (next - position).norm();
        if (!std::isfinite(segment_length) || segment_length <= 0.0 ||
            length_m + segment_length > settings.lines.maximum_length_m) {
            break;
        }
        length_m += segment_length;
        points.push_back(next);
        position = next;
    }
    return points;
}

double coordinate(std::size_t index, std::size_t count, double center, double half_extent) {
    const double fraction = static_cast<double>(index) / static_cast<double>(count - 1U);
    return center - half_extent + 2.0 * half_extent * fraction;
}

void hashDouble(std::uint64_t& hash, double value) noexcept {
    hash ^= std::bit_cast<std::uint64_t>(value);
    hash *= 1'099'511'628'211ULL;
}

void hashVector(std::uint64_t& hash, const math::Vec3d& value) noexcept {
    hashDouble(hash, value.x);
    hashDouble(hash, value.y);
    hashDouble(hash, value.z);
}

} // namespace

std::vector<FieldVectorGlyph> sampleObservedField(const physics::fields::IFieldProvider& provider,
                                                  const FieldVisualizationSettings& settings,
                                                  double time_s) {
    validate(settings);
    if (!std::isfinite(time_s))
        throw std::invalid_argument("field sampling time must be finite in s");
    std::vector<FieldVectorGlyph> glyphs;
    const auto count = settings.vectors.resolution;
    const std::size_t y_count = settings.vectors.geometry == SamplingGeometry::volume ? count : 1U;
    const std::size_t z_count =
        settings.vectors.geometry == SamplingGeometry::plane_xy ? 1U : count;
    const std::size_t x_count =
        settings.vectors.geometry == SamplingGeometry::plane_yz ? 1U : count;
    glyphs.reserve(x_count * y_count * z_count);
    for (std::size_t z = 0; z < z_count; ++z) {
        for (std::size_t y = 0; y < y_count; ++y) {
            for (std::size_t x = 0; x < x_count; ++x) {
                math::Vec3d position = settings.region.center_m;
                if (x_count > 1U)
                    position.x =
                        coordinate(x, x_count, position.x, settings.region.half_extent_m.x);
                if (y_count > 1U)
                    position.y =
                        coordinate(y, y_count, position.y, settings.region.half_extent_m.y);
                if (z_count > 1U)
                    position.z =
                        coordinate(z, z_count, position.z, settings.region.half_extent_m.z);
                const auto sample = provider.sample(position, time_s);
                const auto vector = selectedVector(sample, settings.field);
                const double magnitude = vector.norm();
                if (!sample.valid || !vector.isFinite() || !std::isfinite(magnitude) ||
                    magnitude <= settings.vectors.minimum_magnitude ||
                    magnitude > settings.vectors.maximum_magnitude) {
                    continue;
                }
                double length = settings.vectors.visual_length_m;
                if (settings.vectors.scaling == VectorScaling::linear)
                    length *= magnitude / settings.vectors.reference_magnitude;
                if (settings.vectors.scaling == VectorScaling::logarithmic)
                    length *= std::log10(1.0 + magnitude / settings.vectors.reference_magnitude);
                length = std::clamp(length, settings.vectors.visual_length_m * 0.02,
                                    settings.vectors.visual_length_m * 10.0);
                glyphs.push_back({position, vector / magnitude, magnitude, length});
            }
        }
    }
    return glyphs;
}

std::vector<TracedFieldLine> traceFieldLines(const physics::fields::IFieldProvider& provider,
                                             const FieldVisualizationSettings& settings,
                                             const std::vector<math::Vec3d>& seed_points_m,
                                             double time_s) {
    validate(settings);
    if (!std::isfinite(time_s))
        throw std::invalid_argument("field-line time must be finite in s");
    std::vector<TracedFieldLine> lines;
    std::size_t remaining_work = settings.lines.maximum_total_steps;
    for (const auto& seed : seed_points_m) {
        if (remaining_work == 0U)
            break;
        if (!seed.isFinite() || !insideRegion(seed, settings.region))
            continue;
        std::vector<math::Vec3d> combined;
        if (settings.lines.trace_backward) {
            auto backward = traceDirection(provider, settings, seed, time_s, -1.0, remaining_work);
            combined.assign(backward.rbegin(), backward.rend());
            if (!combined.empty())
                combined.pop_back();
        }
        combined.push_back(seed);
        if (settings.lines.trace_forward) {
            auto forward = traceDirection(provider, settings, seed, time_s, 1.0, remaining_work);
            if (!forward.empty())
                combined.insert(combined.end(), std::next(forward.begin()), forward.end());
        }
        if (combined.size() >= 2U)
            lines.push_back({std::move(combined)});
    }
    return lines;
}

std::vector<math::Vec3d> generateAutomaticFieldSeeds(const core::Scene& scene,
                                                     const FieldVisualizationSettings& settings) {
    validate(settings);
    std::vector<math::Vec3d> seeds;
    const std::size_t desired = settings.lines.automatic_seed_count;
    if (desired == 0U)
        return seeds;
    if (settings.field == ObservedField::electric) {
        std::vector<const core::Body*> charged;
        for (const auto& body : scene.bodies()) {
            if (body.charge_C != 0.0)
                charged.push_back(&body);
        }
        if (charged.empty())
            return seeds;
        seeds.reserve(desired);
        for (std::size_t index = 0; index < desired; ++index) {
            const auto& body = *charged[index % charged.size()];
            const double phase =
                2.0 * std::numbers::pi * static_cast<double>(index) / static_cast<double>(desired);
            const double radius = std::max(body.radius_m * 1.2, settings.lines.step_size_m * 2.0);
            seeds.push_back(body.state.position_m +
                            math::Vec3d{radius * std::cos(phase), radius * std::sin(phase), 0.0});
        }
        return seeds;
    }
    seeds.reserve(desired);
    for (std::size_t index = 0; index < desired; ++index) {
        const double fraction =
            desired == 1U ? 0.5 : static_cast<double>(index) / static_cast<double>(desired - 1U);
        seeds.push_back({settings.region.center_m.x - settings.region.half_extent_m.x +
                             2.0 * settings.region.half_extent_m.x * fraction,
                         settings.region.center_m.y, settings.region.center_m.z});
    }
    return seeds;
}

std::uint64_t fieldVisualizationRevision(const FieldVisualizationSettings& settings) noexcept {
    std::uint64_t hash = 14'695'981'039'346'656'037ULL;
    hashDouble(hash, static_cast<double>(settings.mode));
    hashDouble(hash, static_cast<double>(settings.field));
    hashVector(hash, settings.region.center_m);
    hashVector(hash, settings.region.half_extent_m);
    hashDouble(hash, static_cast<double>(settings.vectors.geometry));
    hashDouble(hash, static_cast<double>(settings.vectors.resolution));
    hashDouble(hash, settings.vectors.visual_length_m);
    hashDouble(hash, static_cast<double>(settings.vectors.scaling));
    hashDouble(hash, settings.vectors.reference_magnitude);
    hashDouble(hash, settings.vectors.minimum_magnitude);
    hashDouble(hash, settings.vectors.maximum_magnitude);
    hashDouble(hash, static_cast<double>(settings.lines.automatic_seed_count));
    hashDouble(hash, settings.lines.step_size_m);
    hashDouble(hash, static_cast<double>(settings.lines.maximum_steps));
    hashDouble(hash, static_cast<double>(settings.lines.maximum_total_steps));
    hashDouble(hash, settings.lines.maximum_length_m);
    hashDouble(hash, settings.lines.minimum_field_magnitude);
    hashDouble(hash, settings.lines.trace_forward ? 1.0 : 0.0);
    hashDouble(hash, settings.lines.trace_backward ? 1.0 : 0.0);
    for (const auto& seed : settings.lines.custom_seeds_m)
        hashVector(hash, seed);
    return hash;
}

} // namespace aetherion::renderer
