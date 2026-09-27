#include "aetherion/core/position_edit_history.hpp"

#include <stdexcept>

namespace aetherion::core {

PositionEditHistory::PositionEditHistory(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0U)
        throw std::invalid_argument("position edit history capacity must be positive");
}

bool PositionEditHistory::select(PositionTarget target, const math::Vec3d& position_m) {
    if (!position_m.isFinite())
        return false;
    if (target_ == target)
        return false;
    target_ = target;
    rebase(position_m);
    return true;
}

void PositionEditHistory::rebase(const math::Vec3d& position_m) {
    if (!position_m.isFinite())
        return;
    initial_m_ = position_m;
    current_m_ = position_m;
    undo_positions_m_.clear();
}

void PositionEditHistory::clear() noexcept {
    target_.reset();
    initial_m_.reset();
    current_m_.reset();
    undo_positions_m_.clear();
}

void PositionEditHistory::rememberCurrent() {
    if (undo_positions_m_.size() == capacity_)
        undo_positions_m_.erase(undo_positions_m_.begin());
    undo_positions_m_.push_back(*current_m_);
}

bool PositionEditHistory::nudge(const math::Vec3d& delta_m) {
    if (!current_m_ || !delta_m.isFinite() || delta_m.squaredNorm() == 0.0)
        return false;
    const auto next = *current_m_ + delta_m;
    if (!next.isFinite())
        return false;
    rememberCurrent();
    current_m_ = next;
    return true;
}

bool PositionEditHistory::undo() {
    if (undo_positions_m_.empty())
        return false;
    current_m_ = undo_positions_m_.back();
    undo_positions_m_.pop_back();
    return true;
}

bool PositionEditHistory::reset() {
    if (!current_m_ || !initial_m_ || *current_m_ == *initial_m_)
        return false;
    rememberCurrent();
    current_m_ = initial_m_;
    return true;
}

} // namespace aetherion::core
