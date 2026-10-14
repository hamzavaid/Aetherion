#include "aetherion/physics/em/retarded_fields.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"
#include "aetherion/physics/em/electrostatics.hpp"

namespace aetherion::physics::em {
namespace {

bool isSource(const core::Body& body) {
    return body.charge_C != 0.0 && body.interactions.contains(core::Interaction::electrostatic);
}

double timeTolerance(const RetardedSettings& settings, double time_s, double delay_s) {
    return settings.absolute_time_tolerance_s + settings.relative_time_tolerance * delay_s +
           8.0 * std::numeric_limits<double>::epsilon() * std::abs(time_s);
}

} // namespace

core::Status validateRetardedSettings(const RetardedSettings& settings) {
    if (!std::isfinite(settings.maximum_observable_distance_m) ||
        settings.maximum_observable_distance_m <= 0.0 ||
        !std::isfinite(settings.absolute_time_tolerance_s) ||
        settings.absolute_time_tolerance_s <= 0.0 ||
        !std::isfinite(settings.relative_time_tolerance) ||
        settings.relative_time_tolerance < 0.0 || settings.relative_time_tolerance > 0.01 ||
        !std::isfinite(settings.minimum_distance_m) || settings.minimum_distance_m < 0.0 ||
        settings.maximum_iterations == 0U || settings.maximum_iterations > 256U)
        return core::Error{
            core::ErrorCode::invalid_argument,
            "retarded settings require finite SI distances/tolerances and 1-256 iterations"};
    return core::success();
}

core::Result<std::size_t> historyCapacity(const RetardedSettings& settings, double physics_dt_s) {
    const auto status = validateRetardedSettings(settings);
    if (!status)
        return status.error();
    if (!std::isfinite(physics_dt_s) || physics_dt_s <= 0.0)
        return core::Error{core::ErrorCode::invalid_argument,
                           "history timestep must be positive seconds"};
    if (!settings.enabled)
        return std::size_t{4};
    const double intervals = std::ceil(
        (settings.maximum_observable_distance_m / constants::speed_of_light_mps) / physics_dt_s);
    if (!std::isfinite(intervals) ||
        intervals > static_cast<double>(maximum_history_samples_per_source - 4U))
        return core::Error{core::ErrorCode::invalid_argument,
                           "retarded domain/timestep exceeds 65536 history samples per source"};
    return static_cast<std::size_t>(intervals) + 4U;
}

core::Status validateRetardedScene(const core::Scene& scene, const RetardedSettings& settings,
                                   double dt_s) {
    const auto capacity = historyCapacity(settings, dt_s);
    if (!capacity)
        return capacity.error();
    if (!settings.enabled)
        return core::success();
    const auto count = static_cast<std::size_t>(
        std::count_if(scene.bodies().begin(), scene.bodies().end(), isSource));
    if (count > maximum_history_sources || count > maximum_total_history_samples / capacity.value())
        return core::Error{core::ErrorCode::invalid_argument,
                           "charged-source history allocation budget exceeded"};
    return core::success();
}

HistoryBuffer::HistoryBuffer(std::size_t capacity) {
    if (capacity < 2U || capacity > maximum_history_samples_per_source)
        throw std::invalid_argument("history capacity must be 2-65536 samples");
    samples_.resize(capacity);
}

const ParticleHistorySample& HistoryBuffer::at(std::size_t index) const {
    return samples_[(begin_ + index) % samples_.size()];
}

const ParticleHistorySample& HistoryBuffer::oldest() const {
    if (size_ == 0U)
        throw std::logic_error("empty source history has no oldest state");
    return at(0);
}

const ParticleHistorySample& HistoryBuffer::newest() const {
    if (size_ == 0U)
        throw std::logic_error("empty source history has no newest state");
    return at(size_ - 1U);
}

core::Status HistoryBuffer::append(const ParticleHistorySample& sample) {
    if (!std::isfinite(sample.time_s) || !sample.position_m.isFinite() ||
        !sample.velocity_mps.isFinite() || !sample.acceleration_mps2.isFinite())
        return core::Error{core::ErrorCode::invalid_argument,
                           "source history requires finite SI state"};
    if (size_ > 0U && sample.time_s < newest().time_s)
        return core::Error{core::ErrorCode::invalid_argument,
                           "source history time cannot run backwards"};
    if (size_ > 0U && sample.time_s == newest().time_s) {
        if (sample == newest())
            return core::success();
        samples_[(begin_ + size_ - 1U) % samples_.size()] = sample;
    } else if (size_ < samples_.size()) {
        samples_[(begin_ + size_) % samples_.size()] = sample;
        ++size_;
    } else {
        samples_[begin_] = sample;
        begin_ = (begin_ + 1U) % samples_.size();
    }
    ++revision_;
    return core::success();
}

core::Result<HistoryBracket> HistoryBuffer::bracket(double time_s) const {
    if (!std::isfinite(time_s) || size_ == 0U || time_s < oldest().time_s ||
        time_s > newest().time_s)
        return core::Error{core::ErrorCode::not_found,
                           "requested source time is outside recorded history"};
    std::size_t low = 0;
    std::size_t high = size_ - 1U;
    while (low < high) {
        const auto middle = low + (high - low) / 2U;
        if (at(middle).time_s < time_s)
            low = middle + 1U;
        else
            high = middle;
    }
    if (at(low).time_s == time_s)
        return HistoryBracket{at(low), at(low)};
    return HistoryBracket{at(low - 1U), at(low)};
}

core::Result<ParticleHistorySample> HistoryBuffer::interpolate(double time_s) const {
    const auto pair = bracket(time_s);
    if (!pair)
        return pair.error();
    const auto& a = pair.value().before;
    const auto& b = pair.value().after;
    if (a.time_s == b.time_s)
        return a;
    const double w = (time_s - a.time_s) / (b.time_s - a.time_s);
    ParticleHistorySample sample{time_s, (1.0 - w) * a.position_m + w * b.position_m,
                                 (1.0 - w) * a.velocity_mps + w * b.velocity_mps,
                                 (1.0 - w) * a.acceleration_mps2 + w * b.acceleration_mps2};
    if (!sample.position_m.isFinite() || !sample.velocity_mps.isFinite() ||
        !sample.acceleration_mps2.isFinite())
        return core::Error{core::ErrorCode::numerical_failure, "trajectory interpolation overflow"};
    return sample;
}

RetardedResult solveRetardedTime(const HistoryBuffer& history, const math::Vec3d& observer_m,
                                 double time_s, const RetardedSettings& settings) {
    RetardedResult result;
    if (!observer_m.isFinite() || !std::isfinite(time_s) || !validateRetardedSettings(settings))
        return result;
    if (history.size() == 0U) {
        result.status = RetardedStatus::history_underrun;
        return result;
    }
    const double c = constants::speed_of_light_mps;
    const double lower = history.oldest().time_s;
    const double upper = history.newest().time_s;
    const double lower_delay = (observer_m - history.oldest().position_m).norm() / c;
    const double upper_delay = (observer_m - history.newest().position_m).norm() / c;
    if (!std::isfinite(lower_delay) || !std::isfinite(upper_delay))
        return result;
    if (lower + lower_delay - time_s > timeTolerance(settings, time_s, lower_delay)) {
        result.status = RetardedStatus::history_underrun;
        return result;
    }
    if (upper + upper_delay - time_s < -timeTolerance(settings, time_s, upper_delay)) {
        result.status = RetardedStatus::history_future;
        return result;
    }
    double guess = std::clamp(time_s - upper_delay, lower, upper);
    for (std::size_t iteration = 1; iteration <= settings.maximum_iterations; ++iteration) {
        result.iterations = iteration;
        const auto pair = history.bracket(guess);
        const auto state = history.interpolate(guess);
        if (!pair || !state)
            return result;
        const auto& a = pair.value().before;
        const auto& b = pair.value().after;
        const double segment_speed =
            a.time_s == b.time_s ? 0.0
                                 : (b.position_m - a.position_m).norm() / (b.time_s - a.time_s);
        if (state.value().velocity_mps.norm() >= c || segment_speed >= c ||
            !std::isfinite(segment_speed)) {
            result.status = RetardedStatus::superluminal_source;
            return result;
        }
        result.source = state.value();
        result.retarded_time_s = guess;
        result.distance_m = (observer_m - result.source.position_m).norm();
        if (!std::isfinite(result.distance_m))
            return result;
        if (result.distance_m <= settings.minimum_distance_m) {
            result.status = RetardedStatus::singularity;
            return result;
        }
        const double delay = result.distance_m / c;
        const double next = time_s - delay;
        result.residual_s = std::abs(next - guess);
        if (result.residual_s <= timeTolerance(settings, time_s, delay)) {
            result.status = RetardedStatus::converged;
            return result;
        }
        guess = std::clamp(next, lower, upper);
    }
    result.status = RetardedStatus::non_converged;
    return result;
}

std::string_view retardedStatusName(RetardedStatus status) noexcept {
    switch (status) {
    case RetardedStatus::converged:
        return "converged";
    case RetardedStatus::history_underrun:
        return "history underrun / warm-up";
    case RetardedStatus::history_future:
        return "history does not reach requested time";
    case RetardedStatus::non_converged:
        return "iteration limit reached";
    case RetardedStatus::singularity:
        return "source-distance guard";
    case RetardedStatus::superluminal_source:
        return "non-subluminal source trajectory";
    case RetardedStatus::invalid_input:
        return "invalid input / interpolation";
    }
    return "invalid";
}

core::Status SourceHistories::configure(const RetardedSettings& settings, double physics_dt_s) {
    const auto capacity = historyCapacity(settings, physics_dt_s);
    if (!capacity)
        return capacity.error();
    if (settings_ == settings && capacity_ == capacity.value() && physics_dt_s_ == physics_dt_s)
        return core::success();
    settings_ = settings;
    capacity_ = capacity.value();
    physics_dt_s_ = physics_dt_s;
    clear();
    return core::success();
}

void SourceHistories::clear() noexcept {
    sources_.clear();
    ++revision_;
}

core::Status SourceHistories::record(const core::Scene& scene, double time_s) {
    if (!settings_.enabled)
        return core::success();
    if (!std::isfinite(time_s))
        return core::Error{core::ErrorCode::invalid_argument,
                           "source recording time must be finite"};
    const auto count = static_cast<std::size_t>(
        std::count_if(scene.bodies().begin(), scene.bodies().end(), isSource));
    if (count > maximum_history_sources || count > maximum_total_history_samples / capacity_)
        return core::Error{core::ErrorCode::invalid_argument,
                           "charged-source history allocation budget exceeded"};
    for (const auto& body : scene.bodies()) {
        if (isSource(body) &&
            (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite() ||
             !body.state.acceleration_mps2.isFinite()))
            return core::Error{core::ErrorCode::numerical_failure,
                               "cannot record nonfinite accepted state"};
    }
    bool changed = std::erase_if(sources_, [&](const auto& entry) {
                       const auto* body = scene.find(entry.first);
                       return body == nullptr || !isSource(*body);
                   }) > 0U;
    for (const auto& body : scene.bodies()) {
        if (!isSource(body))
            continue;
        auto found = sources_.find(body.id);
        if (found != sources_.end() && found->second.charge_C != body.charge_C) {
            sources_.erase(found);
            found = sources_.end();
            changed = true;
        }
        if (found == sources_.end()) {
            found =
                sources_.emplace(body.id, SourceTrajectory{body.charge_C, HistoryBuffer(capacity_)})
                    .first;
            changed = true;
        }
        const auto before = found->second.history.revision();
        const auto status = found->second.history.append(
            {time_s, body.state.position_m, body.state.velocity_mps, body.state.acceleration_mps2});
        if (!status)
            return status;
        changed = changed || before != found->second.history.revision();
    }
    if (changed)
        ++revision_;
    return core::success();
}

const SourceTrajectory* SourceHistories::find(core::EntityId id) const noexcept {
    const auto found = sources_.find(id);
    return found == sources_.end() ? nullptr : &found->second;
}

RetardedFieldProvider::RetardedFieldProvider(const core::Scene& scene,
                                             const ElectromagneticSettings& settings,
                                             const SourceHistories& histories,
                                             const fields::IFieldProvider& instantaneous) noexcept
    : scene_(scene), settings_(settings), histories_(histories), instantaneous_(instantaneous) {}

fields::FieldSample RetardedFieldProvider::sample(const math::Vec3d& position_m,
                                                  double time_s) const {
    ++diagnostics_.queries;
    auto result = instantaneous_.sample(position_m, time_s);
    const auto analytic = sampleAnalyticField(settings_, position_m, time_s);
    result.electric_Vpm = analytic.electric_Vpm;
    result.magnetic_T = analytic.magnetic_T;
    result.valid = analytic.valid;
    result.failure =
        analytic.valid ? fields::SampleFailure::none : fields::SampleFailure::singularity;
    result.retarded = true;
    auto solver_settings = settings_.retarded;
    solver_settings.minimum_distance_m =
        std::max(solver_settings.minimum_distance_m, settings_.minimum_separation_m);
    for (const auto& source : scene_.bodies()) {
        if (!isSource(source))
            continue;
        const auto* trajectory = histories_.find(source.id);
        const auto solved =
            trajectory == nullptr
                ? RetardedResult{.status = RetardedStatus::history_underrun}
                : solveRetardedTime(trajectory->history, position_m, time_s, solver_settings);
        result.maximum_iterations = std::max(result.maximum_iterations, solved.iterations);
        result.maximum_residual_s = std::max(result.maximum_residual_s, solved.residual_s);
        diagnostics_.maximum_iterations_seen =
            std::max(diagnostics_.maximum_iterations_seen, solved.iterations);
        diagnostics_.maximum_residual_s =
            std::max(diagnostics_.maximum_residual_s, solved.residual_s);
        if (solved.status != RetardedStatus::converged) {
            result.valid = false;
            if (solved.status == RetardedStatus::history_underrun ||
                solved.status == RetardedStatus::history_future) {
                ++diagnostics_.history_failures;
                result.failure = fields::SampleFailure::history_unavailable;
            } else if (solved.status == RetardedStatus::non_converged) {
                ++diagnostics_.convergence_failures;
                result.failure = fields::SampleFailure::non_convergence;
            } else if (solved.status == RetardedStatus::singularity) {
                ++diagnostics_.guarded_sources;
                result.failure = fields::SampleFailure::singularity;
            } else {
                ++diagnostics_.invalid_sources;
                result.failure = fields::SampleFailure::invalid_source;
            }
            continue;
        }
        if (solved.distance_m > settings_.retarded.maximum_observable_distance_m) {
            result.valid = false;
            result.failure = fields::SampleFailure::outside_history_domain;
            ++diagnostics_.history_failures;
            continue;
        }
        ++diagnostics_.converged_sources;
        result.maximum_delay_s =
            std::max(result.maximum_delay_s, solved.distance_m / constants::speed_of_light_mps);
        const auto field = samplePointChargeField(trajectory->charge_C, solved.source.position_m,
                                                  position_m, settings_);
        if (field)
            result.electric_Vpm += field.value();
        else {
            result.valid = false;
            result.failure = fields::SampleFailure::singularity;
        }
    }
    if (!result.electric_Vpm.isFinite()) {
        result.valid = false;
        result.failure = fields::SampleFailure::invalid_source;
    }
    if (!result.valid) {
        result.electric_Vpm = {};
        result.magnetic_T = {};
    }
    return result;
}

std::uint64_t RetardedFieldProvider::revision() const noexcept {
    return instantaneous_.revision() ^ (histories_.revision() * 1'099'511'628'211ULL);
}

} // namespace aetherion::physics::em
