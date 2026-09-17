#include "aetherion/serialization/scene_serialization.hpp"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "aetherion/physics/em/electrostatics.hpp"

namespace aetherion::serialization {
namespace {

struct Json;
using Object = std::map<std::string, Json>;
using Array = std::vector<Json>;
struct Json {
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
};

class Parser final {
  public:
    explicit Parser(std::string_view text) : text_(text) {}
    Json parse() {
        auto value = parseValue();
        whitespace();
        if (position_ != text_.size())
            fail("unexpected trailing JSON content");
        return value;
    }

  private:
    [[noreturn]] void fail(const char* message) const { throw std::runtime_error(message); }
    void whitespace() {
        while (position_ < text_.size() && (text_[position_] == ' ' || text_[position_] == '\n' ||
                                            text_[position_] == '\r' || text_[position_] == '\t'))
            ++position_;
    }
    bool consume(char token) {
        whitespace();
        if (position_ < text_.size() && text_[position_] == token) {
            ++position_;
            return true;
        }
        return false;
    }
    Json parseValue() {
        whitespace();
        if (position_ >= text_.size())
            fail("unexpected end of JSON");
        if (text_[position_] == '{')
            return {{parseObject()}};
        if (text_[position_] == '[')
            return {{parseArray()}};
        if (text_[position_] == '"')
            return {{parseString()}};
        if (text_.substr(position_, 4) == "true") {
            position_ += 4;
            return {{true}};
        }
        if (text_.substr(position_, 5) == "false") {
            position_ += 5;
            return {{false}};
        }
        if (text_.substr(position_, 4) == "null") {
            position_ += 4;
            return {{nullptr}};
        }
        return {{parseNumber()}};
    }
    Object parseObject() {
        if (!consume('{'))
            fail("expected JSON object");
        Object object;
        if (consume('}'))
            return object;
        do {
            whitespace();
            if (position_ >= text_.size() || text_[position_] != '"')
                fail("expected JSON object key");
            auto key = parseString();
            if (!consume(':'))
                fail("expected colon after JSON key");
            if (!object.emplace(std::move(key), parseValue()).second)
                fail("duplicate JSON object key");
        } while (consume(','));
        if (!consume('}'))
            fail("expected closing JSON object brace");
        return object;
    }
    Array parseArray() {
        if (!consume('['))
            fail("expected JSON array");
        Array array;
        if (consume(']'))
            return array;
        do {
            array.push_back(parseValue());
        } while (consume(','));
        if (!consume(']'))
            fail("expected closing JSON array bracket");
        return array;
    }
    std::string parseString() {
        if (!consume('"'))
            fail("expected JSON string");
        std::string result;
        while (position_ < text_.size()) {
            const char character = text_[position_++];
            if (character == '"')
                return result;
            if (character == '\\') {
                if (position_ >= text_.size())
                    fail("unterminated JSON escape");
                const char escaped = text_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    result.push_back(escaped);
                else if (escaped == 'n')
                    result.push_back('\n');
                else if (escaped == 'r')
                    result.push_back('\r');
                else if (escaped == 't')
                    result.push_back('\t');
                else
                    fail("unsupported JSON string escape");
            } else {
                result.push_back(character);
            }
        }
        fail("unterminated JSON string");
    }
    double parseNumber() {
        whitespace();
        const char* begin = text_.data() + position_;
        const char* end = text_.data() + text_.size();
        double result = 0.0;
        const auto parsed = std::from_chars(begin, end, result);
        if (parsed.ec != std::errc{} || parsed.ptr == begin || !std::isfinite(result))
            fail("invalid finite JSON number");
        position_ += static_cast<std::size_t>(parsed.ptr - begin);
        return result;
    }

    std::string_view text_;
    std::size_t position_{};
};

const Object& object(const Json& value) {
    const auto* result = std::get_if<Object>(&value.value);
    if (result == nullptr)
        throw std::runtime_error("expected JSON object");
    return *result;
}
const Array& array(const Json& value) {
    const auto* result = std::get_if<Array>(&value.value);
    if (result == nullptr)
        throw std::runtime_error("expected JSON array");
    return *result;
}
const Json& member(const Object& value, const char* key) {
    const auto found = value.find(key);
    if (found == value.end())
        throw std::runtime_error(std::string{"missing JSON member: "} + key);
    return found->second;
}
double number(const Object& value, const char* key) {
    const auto* result = std::get_if<double>(&member(value, key).value);
    if (result == nullptr || !std::isfinite(*result))
        throw std::runtime_error(std::string{"expected finite JSON number: "} + key);
    return *result;
}
bool boolean(const Object& value, const char* key) {
    const auto* result = std::get_if<bool>(&member(value, key).value);
    if (result == nullptr)
        throw std::runtime_error(std::string{"expected JSON boolean: "} + key);
    return *result;
}
bool optionalBoolean(const Object& value, const char* key, bool fallback) {
    const auto found = value.find(key);
    if (found == value.end())
        return fallback;
    const auto* result = std::get_if<bool>(&found->second.value);
    if (result == nullptr)
        throw std::runtime_error(std::string{"expected JSON boolean: "} + key);
    return *result;
}
std::string string(const Object& value, const char* key) {
    const auto* result = std::get_if<std::string>(&member(value, key).value);
    if (result == nullptr)
        throw std::runtime_error(std::string{"expected JSON string: "} + key);
    return *result;
}
std::size_t sizeValue(const Object& value, const char* key) {
    const double input = number(value, key);
    if (input < 0.0 || std::floor(input) != input ||
        input > static_cast<double>(std::numeric_limits<std::size_t>::max()))
        throw std::runtime_error(std::string{"expected non-negative integer: "} + key);
    return static_cast<std::size_t>(input);
}
math::Vec3d vector(const Json& value) {
    const auto& values = array(value);
    if (values.size() != 3U)
        throw std::runtime_error("3D vector JSON array must have three entries");
    math::Vec3d result;
    double* destinations[] = {&result.x, &result.y, &result.z};
    for (std::size_t index = 0; index < 3U; ++index) {
        const auto* component = std::get_if<double>(&values[index].value);
        if (component == nullptr || !std::isfinite(*component))
            throw std::runtime_error("3D vector components must be finite numbers");
        *destinations[index] = *component;
    }
    return result;
}

void writeEscaped(std::ostream& output, std::string_view value) {
    output << '"';
    for (const char character : value) {
        if (character == '"' || character == '\\')
            output << '\\' << character;
        else if (character == '\n')
            output << "\\n";
        else if (character == '\r')
            output << "\\r";
        else if (character == '\t')
            output << "\\t";
        else
            output << character;
    }
    output << '"';
}
void writeVector(std::ostream& output, const math::Vec3d& value) {
    output << '[' << value.x << ',' << value.y << ',' << value.z << ']';
}
const char* boolText(bool value) { return value ? "true" : "false"; }

template <typename Enum> Enum enumValue(const Object& value, const char* key, int maximum) {
    const double raw = number(value, key);
    if (std::floor(raw) != raw || raw < 0.0 || raw > static_cast<double>(maximum))
        throw std::runtime_error(std::string{"invalid enum JSON value: "} + key);
    return static_cast<Enum>(static_cast<int>(raw));
}

} // namespace

std::string serializeScene(const SceneDocument& document) {
    std::ostringstream out;
    out << std::setprecision(17);
    const auto& runtime = document.runtime;
    const auto& em = runtime.electromagnetism;
    const auto& visual = document.visualization;
    const auto& field = visual.field_visualization;
    out << "{\n\"schemaVersion\":" << current_scene_schema_version << ",\n";
    out << "\"simulation\":{\"physicsDt\":" << runtime.physics_dt_s
        << ",\"timeScale\":" << runtime.time_scale << ",\"maxSubsteps\":" << runtime.max_substeps
        << ",\"gravity\":" << boolText(runtime.gravity_enabled)
        << ",\"integrator\":" << static_cast<int>(runtime.integrator)
        << ",\"electromagnetism\":{\"electrostatics\":" << boolText(em.electrostatics_enabled)
        << ",\"magnetic\":" << boolText(em.magnetic_enabled)
        << ",\"minimumSeparation\":" << em.minimum_separation_m
        << ",\"softening\":" << em.softening_m << ",\"sources\":[";
    for (std::size_t index = 0; index < em.analytic_sources.size(); ++index) {
        if (index > 0U)
            out << ',';
        const auto& source = em.analytic_sources[index];
        out << "{\"kind\":" << static_cast<int>(source.kind) << ",\"position\":";
        writeVector(out, source.position_m);
        out << ",\"electric\":";
        writeVector(out, source.electric_Vpm);
        out << ",\"magnetic\":";
        writeVector(out, source.magnetic_T);
        out << ",\"dipoleMoment\":";
        writeVector(out, source.magnetic_dipole_moment_Am2);
        out << ",\"singularityRadius\":" << source.singularity_radius_m << '}';
    }
    out << "]}},\n\"bodies\":[";
    for (std::size_t index = 0; index < document.scene.size(); ++index) {
        if (index > 0U)
            out << ',';
        const auto& body = document.scene.bodies()[index];
        out << "{\"id\":" << body.id << ",\"name\":";
        writeEscaped(out, body.name);
        out << ",\"mass\":" << body.mass_kg << ",\"charge\":" << body.charge_C
            << ",\"radius\":" << body.radius_m << ",\"fixed\":" << boolText(body.fixed)
            << ",\"interactions\":" << body.interactions.bits << ",\"position\":";
        writeVector(out, body.state.position_m);
        out << ",\"velocity\":";
        writeVector(out, body.state.velocity_mps);
        out << ",\"acceleration\":";
        writeVector(out, body.state.acceleration_mps2);
        out << '}';
    }
    out << "],\n\"visualization\":{\"metersToRenderUnit\":" << visual.meters_to_render_units
        << ",\"minimumApparentRadius\":" << visual.minimum_apparent_radius
        << ",\"bodyRadiusScale\":" << visual.body_radius_scale
        << ",\"showGrid\":" << boolText(visual.show_grid)
        << ",\"trails\":" << boolText(visual.trails_enabled)
        << ",\"trailDuration\":" << visual.trail_duration_s
        << ",\"field\":{\"mode\":" << static_cast<int>(field.mode)
        << ",\"type\":" << static_cast<int>(field.field)
        << ",\"planar2d\":" << boolText(field.planar_2d) << ",\"center\":";
    writeVector(out, field.region.center_m);
    out << ",\"halfExtent\":";
    writeVector(out, field.region.half_extent_m);
    out << ",\"vectors\":{\"geometry\":" << static_cast<int>(field.vectors.geometry)
        << ",\"resolution\":" << field.vectors.resolution
        << ",\"length\":" << field.vectors.visual_length_m
        << ",\"scaling\":" << static_cast<int>(field.vectors.scaling)
        << ",\"reference\":" << field.vectors.reference_magnitude
        << ",\"minimum\":" << field.vectors.minimum_magnitude
        << ",\"maximum\":" << field.vectors.maximum_magnitude
        << "},\"lines\":{\"automaticSeeds\":" << field.lines.automatic_seed_count
        << ",\"stepSize\":" << field.lines.step_size_m
        << ",\"maximumSteps\":" << field.lines.maximum_steps
        << ",\"maximumTotalSteps\":" << field.lines.maximum_total_steps
        << ",\"maximumLength\":" << field.lines.maximum_length_m
        << ",\"minimumField\":" << field.lines.minimum_field_magnitude
        << ",\"forward\":" << boolText(field.lines.trace_forward)
        << ",\"backward\":" << boolText(field.lines.trace_backward) << ",\"customSeeds\":[";
    for (std::size_t index = 0; index < field.lines.custom_seeds_m.size(); ++index) {
        if (index > 0U)
            out << ',';
        writeVector(out, field.lines.custom_seeds_m[index]);
    }
    out << "]}}}}\n";
    return out.str();
}

core::Result<SceneDocument> deserializeScene(std::string_view json) {
    try {
        const auto root = object(Parser(json).parse());
        if (number(root, "schemaVersion") != current_scene_schema_version)
            throw std::runtime_error("unsupported scene schemaVersion");
        SceneDocument document;
        const auto& simulation = object(member(root, "simulation"));
        document.runtime.physics_dt_s = number(simulation, "physicsDt");
        document.runtime.time_scale = number(simulation, "timeScale");
        document.runtime.max_substeps = sizeValue(simulation, "maxSubsteps");
        document.runtime.gravity_enabled = boolean(simulation, "gravity");
        document.runtime.integrator =
            enumValue<physics::IntegratorKind>(simulation, "integrator", 3);
        const auto& em = object(member(simulation, "electromagnetism"));
        document.runtime.electromagnetism.electrostatics_enabled = boolean(em, "electrostatics");
        document.runtime.electromagnetism.magnetic_enabled = boolean(em, "magnetic");
        document.runtime.electromagnetism.minimum_separation_m = number(em, "minimumSeparation");
        document.runtime.electromagnetism.softening_m = number(em, "softening");
        for (const auto& source_value : array(member(em, "sources"))) {
            const auto& source_json = object(source_value);
            physics::em::AnalyticFieldSource source;
            source.kind = enumValue<physics::em::AnalyticFieldSourceKind>(source_json, "kind", 1);
            source.position_m = vector(member(source_json, "position"));
            source.electric_Vpm = vector(member(source_json, "electric"));
            source.magnetic_T = vector(member(source_json, "magnetic"));
            source.magnetic_dipole_moment_Am2 = vector(member(source_json, "dipoleMoment"));
            source.singularity_radius_m = number(source_json, "singularityRadius");
            document.runtime.electromagnetism.analytic_sources.push_back(source);
        }
        for (const auto& body_value : array(member(root, "bodies"))) {
            const auto& body_json = object(body_value);
            core::Body body;
            body.id = static_cast<core::EntityId>(sizeValue(body_json, "id"));
            body.name = string(body_json, "name");
            body.mass_kg = number(body_json, "mass");
            body.charge_C = number(body_json, "charge");
            body.radius_m = number(body_json, "radius");
            body.fixed = boolean(body_json, "fixed");
            body.interactions.bits =
                static_cast<std::uint32_t>(sizeValue(body_json, "interactions"));
            body.state.position_m = vector(member(body_json, "position"));
            body.state.velocity_mps = vector(member(body_json, "velocity"));
            body.state.acceleration_mps2 = vector(member(body_json, "acceleration"));
            const auto imported = document.scene.importBody(std::move(body));
            if (!imported)
                return imported.error();
        }
        const auto& visual = object(member(root, "visualization"));
        document.visualization.meters_to_render_units = number(visual, "metersToRenderUnit");
        document.visualization.minimum_apparent_radius =
            static_cast<float>(number(visual, "minimumApparentRadius"));
        document.visualization.body_radius_scale =
            static_cast<float>(number(visual, "bodyRadiusScale"));
        document.visualization.show_grid = boolean(visual, "showGrid");
        document.visualization.trails_enabled = boolean(visual, "trails");
        document.visualization.trail_duration_s = number(visual, "trailDuration");
        auto& field = document.visualization.field_visualization;
        const auto& field_json = object(member(visual, "field"));
        field.mode = enumValue<renderer::FieldDisplayMode>(field_json, "mode", 2);
        field.field = enumValue<renderer::ObservedField>(field_json, "type", 1);
        field.planar_2d = optionalBoolean(field_json, "planar2d", false);
        field.region.center_m = vector(member(field_json, "center"));
        field.region.half_extent_m = vector(member(field_json, "halfExtent"));
        const auto& vectors = object(member(field_json, "vectors"));
        field.vectors.geometry = enumValue<renderer::SamplingGeometry>(vectors, "geometry", 3);
        field.vectors.resolution = sizeValue(vectors, "resolution");
        field.vectors.visual_length_m = number(vectors, "length");
        field.vectors.scaling = enumValue<renderer::VectorScaling>(vectors, "scaling", 2);
        field.vectors.reference_magnitude = number(vectors, "reference");
        field.vectors.minimum_magnitude = number(vectors, "minimum");
        field.vectors.maximum_magnitude = number(vectors, "maximum");
        const auto& lines = object(member(field_json, "lines"));
        field.lines.automatic_seed_count = sizeValue(lines, "automaticSeeds");
        field.lines.step_size_m = number(lines, "stepSize");
        field.lines.maximum_steps = sizeValue(lines, "maximumSteps");
        field.lines.maximum_total_steps = sizeValue(lines, "maximumTotalSteps");
        field.lines.maximum_length_m = number(lines, "maximumLength");
        field.lines.minimum_field_magnitude = number(lines, "minimumField");
        field.lines.trace_forward = boolean(lines, "forward");
        field.lines.trace_backward = boolean(lines, "backward");
        for (const auto& seed : array(member(lines, "customSeeds")))
            field.lines.custom_seeds_m.push_back(vector(seed));
        const auto em_status =
            physics::em::validateElectromagneticSettings(document.runtime.electromagnetism);
        if (!em_status)
            return em_status.error();
        return document;
    } catch (const std::exception& error) {
        return core::Error{core::ErrorCode::invalid_argument,
                           std::string{"scene JSON parse failed: "} + error.what()};
    }
}

core::Status saveSceneFile(const std::filesystem::path& path, const SceneDocument& document) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        return core::Error{core::ErrorCode::platform_failure,
                           "could not open scene file for write"};
    output << serializeScene(document);
    if (!output)
        return core::Error{core::ErrorCode::platform_failure, "failed while writing scene file"};
    return core::success();
}

core::Result<SceneDocument> loadSceneFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return core::Error{core::ErrorCode::not_found, "could not open scene file"};
    std::ostringstream content;
    content << input.rdbuf();
    if (!input.good() && !input.eof())
        return core::Error{core::ErrorCode::platform_failure, "failed while reading scene file"};
    return deserializeScene(content.str());
}

} // namespace aetherion::serialization
