#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/render_data.hpp"

namespace aetherion::serialization {

inline constexpr int current_scene_schema_version = 1;

struct SceneDocument {
    core::Scene scene;
    core::RuntimeSettings runtime;
    renderer::RenderSettings visualization;
};

/// Emits deterministic, human-readable schema-versioned JSON with SI-valued state.
[[nodiscard]] std::string serializeScene(const SceneDocument& document);
/// Parses schema v1 JSON and rejects missing, mistyped, non-finite, or invalid physical values.
[[nodiscard]] core::Result<SceneDocument> deserializeScene(std::string_view json);
[[nodiscard]] core::Status saveSceneFile(const std::filesystem::path& path,
                                         const SceneDocument& document);
[[nodiscard]] core::Result<SceneDocument> loadSceneFile(const std::filesystem::path& path);

} // namespace aetherion::serialization
