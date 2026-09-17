#include "aetherion/renderer/opengl_backend.hpp"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "aetherion/renderer/adaptive_grid.hpp"
#include "aetherion/renderer/mesh.hpp"
#include "aetherion/renderer/trail_history.hpp"

namespace aetherion::renderer {
namespace {

constexpr const char* sphere_vertex_shader = R"(
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 iPosition;
layout(location = 3) in float iRadius;
layout(location = 4) in vec3 iColor;
layout(location = 5) in float iHighlighted;
uniform mat4 uViewProjection;
out vec3 vNormal;
out vec3 vColor;
out float vHighlighted;
void main() {
    gl_Position = uViewProjection * vec4(iPosition + aPosition * iRadius, 1.0);
    vNormal = aNormal;
    vColor = iColor;
    vHighlighted = iHighlighted;
}
)";

constexpr const char* sphere_fragment_shader = R"(
#version 410 core
in vec3 vNormal;
in vec3 vColor;
in float vHighlighted;
out vec4 fragmentColor;
void main() {
    vec3 lightDirection = normalize(vec3(0.35, 0.8, 0.45));
    float diffuse = max(dot(normalize(vNormal), lightDirection), 0.0);
    vec3 shaded = vColor * (0.18 + 0.82 * diffuse);
    vec3 highlighted = mix(shaded, vec3(1.0, 0.68, 0.08), 0.82) + vec3(0.12);
    fragmentColor = vec4(mix(shaded, highlighted, vHighlighted), 1.0);
}
)";

constexpr const char* grid_vertex_shader = R"(
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 uViewProjection;
uniform vec3 uGridOrigin;
out vec3 vColor;
void main() {
    gl_Position = uViewProjection * vec4(aPosition + uGridOrigin, 1.0);
    vColor = aColor;
}
)";

constexpr const char* grid_fragment_shader = R"(
#version 410 core
in vec3 vColor;
out vec4 fragmentColor;
void main() { fragmentColor = vec4(vColor, 1.0); }
)";

struct InstanceGpu {
    std::array<float, 3> position;
    float radius{};
    std::array<float, 3> color;
    float highlighted{};
};

struct GridVertexGpu {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

core::Result<GLuint> compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    std::string log(static_cast<std::size_t>(std::max(log_length, 1)), '\0');
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
    glDeleteShader(shader);
    return core::Error{core::ErrorCode::platform_failure,
                       "OpenGL shader compilation failed: " + log};
}

core::Result<GLuint> createProgram(const char* vertex_source, const char* fragment_source) {
    const auto vertex = compileShader(GL_VERTEX_SHADER, vertex_source);
    if (!vertex) {
        return vertex.error();
    }
    const auto fragment = compileShader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment) {
        glDeleteShader(vertex.value());
        return fragment.error();
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex.value());
    glAttachShader(program, fragment.value());
    glLinkProgram(program);
    glDeleteShader(vertex.value());
    glDeleteShader(fragment.value());
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    std::string log(static_cast<std::size_t>(std::max(log_length, 1)), '\0');
    glGetProgramInfoLog(program, log_length, nullptr, log.data());
    glDeleteProgram(program);
    return core::Error{core::ErrorCode::platform_failure, "OpenGL program link failed: " + log};
}

} // namespace

class OpenGlRenderer::Impl final {
  public:
    ~Impl() { shutdown(); }

    core::Status initialize(int width, int height, std::string_view title, bool visible) {
        if (width <= 0 || height <= 0 || title.empty()) {
            return core::Error{core::ErrorCode::invalid_argument,
                               "renderer requires positive dimensions and a non-empty title"};
        }
        if (glfwInit() != GLFW_TRUE) {
            return core::Error{core::ErrorCode::platform_failure, "GLFW initialization failed"};
        }
        glfw_initialized_ = true;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
        const std::string owned_title(title);
        window_ = glfwCreateWindow(width, height, owned_title.c_str(), nullptr, nullptr);
        if (window_ == nullptr) {
            return core::Error{core::ErrorCode::platform_failure,
                               "GLFW could not create an OpenGL 4.1 core window"};
        }
        glfwMakeContextCurrent(window_);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
            return core::Error{core::ErrorCode::platform_failure,
                               "OpenGL 4.1 function loading failed"};
        }
        glfwSwapInterval(1);
        glfwSetWindowUserPointer(window_, this);
        glfwSetCursorPosCallback(window_, cursorCallback);
        glfwSetMouseButtonCallback(window_, mouseButtonCallback);
        glfwSetScrollCallback(window_, scrollCallback);
        glfwSetKeyCallback(window_, keyCallback);

        const auto sphere_program = createProgram(sphere_vertex_shader, sphere_fragment_shader);
        if (!sphere_program) {
            return sphere_program.error();
        }
        sphere_program_ = sphere_program.value();
        const auto grid_program = createProgram(grid_vertex_shader, grid_fragment_shader);
        if (!grid_program) {
            return grid_program.error();
        }
        grid_program_ = grid_program.value();
        createSphereResources();
        createGridResources();
        createTrailResources();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        return core::success();
    }

    core::Status render(const core::Scene& scene, Camera& camera, const RenderSettings& settings,
                        const physics::fields::IFieldProvider* fields) {
        if (window_ == nullptr) {
            return core::Error{core::ErrorCode::platform_failure, "renderer is not initialized"};
        }
        active_camera_ = &camera;
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        if (width <= 0 || height <= 0) {
            return core::success();
        }
        const double aspect = static_cast<double>(width) / static_cast<double>(height);
        const auto matrix = camera.viewProjection(aspect, settings.meters_to_render_units);
        const auto instances = buildBodyInstances(scene, camera.positionWorld(), settings);
        std::vector<InstanceGpu> gpu_instances;
        gpu_instances.reserve(instances.size());
        for (const auto& instance : instances) {
            gpu_instances.push_back(
                {{instance.position.x, instance.position.y, instance.position.z},
                 instance.radius,
                 {instance.color.x, instance.color.y, instance.color.z},
                 instance.highlighted ? 1.0F : 0.0F});
        }

        if (settings.trails_enabled) {
            trail_history_.sample(scene, settings.simulation_time_s, settings.trail_duration_s);
        } else {
            trail_history_.clear();
        }
        if (settings.show_grid)
            updateGridResources(camera, settings);

        glViewport(0, 0, width, height);
        glClearColor(0.018F, 0.026F, 0.045F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (settings.show_grid) {
            glUseProgram(grid_program_);
            glUniformMatrix4fv(glGetUniformLocation(grid_program_, "uViewProjection"), 1, GL_FALSE,
                               matrix.data());
            glUniform3f(glGetUniformLocation(grid_program_, "uGridOrigin"), grid_origin_.x,
                        grid_origin_.y, grid_origin_.z);
            glBindVertexArray(grid_vao_);
            glDrawArrays(GL_LINES, 0, grid_vertex_count_);
        }

        drawTrails(scene, camera, settings, matrix);
        updateFieldVisualization(scene, settings, fields);
        drawFieldVisualization(camera, settings, matrix);

        if (!gpu_instances.empty()) {
            glUseProgram(sphere_program_);
            glUniformMatrix4fv(glGetUniformLocation(sphere_program_, "uViewProjection"), 1,
                               GL_FALSE, matrix.data());
            glBindVertexArray(sphere_vao_);
            glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(gpu_instances.size() * sizeof(InstanceGpu)),
                         gpu_instances.data(), GL_STREAM_DRAW);
            glDrawElementsInstanced(GL_TRIANGLES, sphere_index_count_, GL_UNSIGNED_INT, nullptr,
                                    static_cast<GLsizei>(gpu_instances.size()));
        }
        glBindVertexArray(0);
        if (glGetError() != GL_NO_ERROR) {
            return core::Error{core::ErrorCode::platform_failure,
                               "OpenGL reported an error while rendering the scene"};
        }
        return core::success();
    }

    void present() {
        if (window_ != nullptr) {
            glfwSwapBuffers(window_);
        }
    }

    void pollEvents() { glfwPollEvents(); }
    void setInputCapture(const InputCapture& capture) noexcept {
        input_capture_ = capture;
        if (!routesMouseToScene(input_capture_)) {
            orbiting_ = false;
            panning_ = false;
            left_pressed_in_scene_ = false;
        }
    }
    std::optional<ViewportClick> takeViewportClick() noexcept {
        auto click = pending_viewport_click_;
        pending_viewport_click_.reset();
        return click;
    }
    bool shouldClose() const noexcept {
        return window_ == nullptr || glfwWindowShouldClose(window_) != 0;
    }
    void requestClose() noexcept {
        if (window_ != nullptr) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }
    void* nativeWindowHandle() noexcept { return window_; }

  private:
    static Impl* fromWindow(GLFWwindow* window) noexcept {
        return static_cast<Impl*>(glfwGetWindowUserPointer(window));
    }

    static void cursorCallback(GLFWwindow* window, double x, double y) {
        auto* self = fromWindow(window);
        if (self == nullptr || self->active_camera_ == nullptr) {
            return;
        }
        if (!self->has_cursor_) {
            self->last_cursor_x_ = x;
            self->last_cursor_y_ = y;
            self->has_cursor_ = true;
            return;
        }
        const double delta_x = x - self->last_cursor_x_;
        const double delta_y = y - self->last_cursor_y_;
        self->last_cursor_x_ = x;
        self->last_cursor_y_ = y;
        if (!routesMouseToScene(self->input_capture_)) {
            return;
        }
        if (self->left_pressed_in_scene_) {
            const double press_delta_x = x - self->left_press_x_;
            const double press_delta_y = y - self->left_press_y_;
            self->left_drag_distance_squared_ =
                std::max(self->left_drag_distance_squared_,
                         press_delta_x * press_delta_x + press_delta_y * press_delta_y);
        }
        if (self->orbiting_) {
            self->active_camera_->orbit(-delta_x * 0.005, -delta_y * 0.005);
        }
        if (self->panning_) {
            const double meters_per_pixel = self->active_camera_->distanceMeters() * 0.002;
            self->active_camera_->pan(-delta_x * meters_per_pixel, delta_y * meters_per_pixel);
        }
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*modifiers*/) {
        auto* self = fromWindow(window);
        if (self == nullptr) {
            return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                if (!routesMouseToScene(self->input_capture_))
                    return;
                glfwGetCursorPos(window, &self->left_press_x_, &self->left_press_y_);
                self->last_cursor_x_ = self->left_press_x_;
                self->last_cursor_y_ = self->left_press_y_;
                self->has_cursor_ = true;
                self->left_drag_distance_squared_ = 0.0;
                self->left_pressed_in_scene_ = true;
                self->orbiting_ = true;
            } else if (action == GLFW_RELEASE) {
                self->orbiting_ = false;
                if (self->left_pressed_in_scene_ && routesMouseToScene(self->input_capture_) &&
                    self->left_drag_distance_squared_ <= 16.0) {
                    int window_width = 0;
                    int window_height = 0;
                    int framebuffer_width = 0;
                    int framebuffer_height = 0;
                    glfwGetWindowSize(window, &window_width, &window_height);
                    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
                    if (window_width > 0 && window_height > 0 && framebuffer_width > 0 &&
                        framebuffer_height > 0) {
                        double cursor_x = 0.0;
                        double cursor_y = 0.0;
                        glfwGetCursorPos(window, &cursor_x, &cursor_y);
                        self->pending_viewport_click_ = {
                            .x_ndc = 2.0 * cursor_x / static_cast<double>(window_width) - 1.0,
                            .y_ndc = 1.0 - 2.0 * cursor_y / static_cast<double>(window_height),
                            .aspect_ratio = static_cast<double>(framebuffer_width) /
                                            static_cast<double>(framebuffer_height)};
                    }
                }
                self->left_pressed_in_scene_ = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS && routesMouseToScene(self->input_capture_))
                self->panning_ = true;
            if (action == GLFW_RELEASE)
                self->panning_ = false;
        }
    }

    static void scrollCallback(GLFWwindow* window, double /*x_offset*/, double y_offset) {
        auto* self = fromWindow(window);
        if (self != nullptr && self->active_camera_ != nullptr &&
            routesMouseToScene(self->input_capture_)) {
            self->active_camera_->zoom(-y_offset);
        }
    }

    static void keyCallback(GLFWwindow* window, int key, int /*scan_code*/, int action,
                            int /*modifiers*/) {
        auto* self = fromWindow(window);
        if (self != nullptr && self->active_camera_ != nullptr &&
            routesKeyboardToScene(self->input_capture_) && action == GLFW_PRESS &&
            key == GLFW_KEY_R) {
            self->active_camera_->reset();
        }
    }

    void createSphereResources() {
        const auto sphere = makeUvSphere(48, 24);
        sphere_index_count_ = static_cast<GLsizei>(sphere.indices.size());
        glGenVertexArrays(1, &sphere_vao_);
        glGenBuffers(1, &sphere_vbo_);
        glGenBuffers(1, &sphere_ebo_);
        glGenBuffers(1, &instance_vbo_);
        glBindVertexArray(sphere_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(sphere.vertices.size() * sizeof(SphereVertex)),
                     sphere.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(sphere.indices.size() * sizeof(std::uint32_t)),
                     sphere.indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex),
                              reinterpret_cast<void*>(3U * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceGpu), nullptr);
        glVertexAttribDivisor(2, 1);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceGpu),
                              reinterpret_cast<void*>(3U * sizeof(float)));
        glVertexAttribDivisor(3, 1);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceGpu),
                              reinterpret_cast<void*>(4U * sizeof(float)));
        glVertexAttribDivisor(4, 1);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceGpu),
                              reinterpret_cast<void*>(7U * sizeof(float)));
        glVertexAttribDivisor(5, 1);
    }

    void createGridResources() {
        glGenVertexArrays(1, &grid_vao_);
        glGenBuffers(1, &grid_vbo_);
        glBindVertexArray(grid_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, grid_vbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu),
                              reinterpret_cast<void*>(3U * sizeof(float)));
        glBindVertexArray(0);
    }

    void updateGridResources(const Camera& camera, const RenderSettings& settings) {
        const auto parameters =
            calculateAdaptiveGrid(camera.distanceMeters(), settings.meters_to_render_units);
        const double spacing_m = parameters.spacing_render_units / settings.meters_to_render_units;
        const auto& target = camera.targetWorld();
        const math::Vec3d center_world_m{std::floor(target.x / spacing_m) * spacing_m, 0.0,
                                         std::floor(target.z / spacing_m) * spacing_m};
        grid_origin_ = toCameraRelative(center_world_m, camera.positionWorld(),
                                        settings.meters_to_render_units);
        std::vector<GridVertexGpu> vertices;
        const auto subdivisions = static_cast<int>(parameters.subdivisions_each_direction);
        vertices.reserve(static_cast<std::size_t>(4 * (2 * subdivisions + 1)));
        const float extent = static_cast<float>(parameters.half_extent_render_units);
        const float spacing = static_cast<float>(parameters.spacing_render_units);
        for (int index = -subdivisions; index <= subdivisions; ++index) {
            const float coordinate = static_cast<float>(index) * spacing;
            const bool major = index % 10 == 0;
            const std::array<float, 3> color =
                major ? std::array{0.32F, 0.44F, 0.58F} : std::array{0.09F, 0.13F, 0.18F};
            vertices.push_back({{coordinate, 0.0F, -extent}, color});
            vertices.push_back({{coordinate, 0.0F, extent}, color});
            vertices.push_back({{-extent, 0.0F, coordinate}, color});
            vertices.push_back({{extent, 0.0F, coordinate}, color});
        }
        grid_vertex_count_ = static_cast<GLsizei>(vertices.size());
        glBindVertexArray(grid_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, grid_vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(GridVertexGpu)),
                     vertices.data(), GL_STREAM_DRAW);
        glBindVertexArray(0);
    }

    void createTrailResources() {
        glGenVertexArrays(1, &trail_vao_);
        glGenBuffers(1, &trail_vbo_);
        glBindVertexArray(trail_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, trail_vbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu),
                              reinterpret_cast<void*>(3U * sizeof(float)));
        glBindVertexArray(0);
    }

    void drawTrails(const core::Scene& scene, const Camera& camera, const RenderSettings& settings,
                    const Mat4f& matrix) {
        if (!settings.trails_enabled)
            return;
        glUseProgram(grid_program_);
        glUniformMatrix4fv(glGetUniformLocation(grid_program_, "uViewProjection"), 1, GL_FALSE,
                           matrix.data());
        glUniform3f(glGetUniformLocation(grid_program_, "uGridOrigin"), 0.0F, 0.0F, 0.0F);
        glBindVertexArray(trail_vao_);
        for (const auto& [id, trail] : trail_history_.trails()) {
            if (trail.points.size() < 2U || scene.find(id) == nullptr)
                continue;
            const bool selected = settings.selected_entity == id;
            const std::array<float, 3> color =
                selected ? std::array{1.0F, 0.68F, 0.08F} : std::array{0.18F, 0.62F, 0.95F};
            std::vector<GridVertexGpu> vertices;
            vertices.reserve(trail.points.size());
            for (const auto& point : trail.points) {
                const auto relative =
                    toCameraRelative(point.position_world_m, camera.positionWorld(),
                                     settings.meters_to_render_units);
                vertices.push_back({{relative.x, relative.y, relative.z}, color});
            }
            glBindBuffer(GL_ARRAY_BUFFER, trail_vbo_);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vertices.size() * sizeof(GridVertexGpu)),
                         vertices.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices.size()));
        }
        glBindVertexArray(0);
    }

    void updateFieldVisualization(const core::Scene& scene, const RenderSettings& settings,
                                  const physics::fields::IFieldProvider* fields) {
        if (fields == nullptr || settings.field_visualization.mode == FieldDisplayMode::none) {
            field_glyphs_.clear();
            field_lines_.clear();
            field_cache_valid_ = false;
            return;
        }
        const std::uint64_t revision =
            fields->revision() ^ fieldVisualizationRevision(settings.field_visualization);
        if (field_cache_valid_ && revision == field_cache_revision_)
            return;
        field_cache_valid_ = true;
        field_cache_revision_ = revision;
        field_glyphs_.clear();
        field_lines_.clear();
        if (settings.field_visualization.mode == FieldDisplayMode::observed_vectors) {
            field_glyphs_ = sampleObservedField(*fields, settings.field_visualization,
                                                settings.simulation_time_s);
            return;
        }
        auto seeds = settings.field_visualization.lines.custom_seeds_m;
        auto automatic = generateAutomaticFieldSeeds(scene, settings.field_visualization);
        seeds.insert(seeds.end(), automatic.begin(), automatic.end());
        field_lines_ = traceFieldLines(*fields, settings.field_visualization, seeds,
                                       settings.simulation_time_s);
    }

    void uploadAndDrawLineVertices(const std::vector<GridVertexGpu>& vertices, GLenum primitive) {
        if (vertices.size() < 2U)
            return;
        glBindBuffer(GL_ARRAY_BUFFER, trail_vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(GridVertexGpu)),
                     vertices.data(), GL_STREAM_DRAW);
        glDrawArrays(primitive, 0, static_cast<GLsizei>(vertices.size()));
    }

    void drawFieldVisualization(const Camera& camera, const RenderSettings& settings,
                                const Mat4f& matrix) {
        if (settings.field_visualization.mode == FieldDisplayMode::none)
            return;
        glUseProgram(grid_program_);
        glUniformMatrix4fv(glGetUniformLocation(grid_program_, "uViewProjection"), 1, GL_FALSE,
                           matrix.data());
        glUniform3f(glGetUniformLocation(grid_program_, "uGridOrigin"), 0.0F, 0.0F, 0.0F);
        glBindVertexArray(trail_vao_);
        const std::array<float, 3> color =
            settings.field_visualization.field == ObservedField::electric
                ? std::array{1.0F, 0.38F, 0.08F}
                : std::array{0.72F, 0.28F, 1.0F};
        if (settings.field_visualization.mode == FieldDisplayMode::observed_vectors) {
            std::vector<GridVertexGpu> vertices;
            vertices.reserve(field_glyphs_.size() * 6U);
            for (const auto& glyph : field_glyphs_) {
                const auto end = glyph.position_m + glyph.direction * glyph.visual_length_m;
                auto side = math::cross(glyph.direction, {0.0, 1.0, 0.0});
                if (side.squaredNorm() < 1.0e-12)
                    side = math::cross(glyph.direction, {1.0, 0.0, 0.0});
                side = side.normalized();
                const auto wing_base = end - glyph.direction * (0.25 * glyph.visual_length_m);
                const auto wing_offset = side * (0.12 * glyph.visual_length_m);
                const std::array<math::Vec3d, 6> points = {
                    glyph.position_m,        end, end,
                    wing_base + wing_offset, end, wing_base - wing_offset};
                for (const auto& point : points) {
                    const auto relative = toCameraRelative(point, camera.positionWorld(),
                                                           settings.meters_to_render_units);
                    vertices.push_back({{relative.x, relative.y, relative.z}, color});
                }
            }
            uploadAndDrawLineVertices(vertices, GL_LINES);
        } else {
            for (const auto& line : field_lines_) {
                std::vector<GridVertexGpu> vertices;
                vertices.reserve(line.points_m.size());
                for (const auto& point : line.points_m) {
                    const auto relative = toCameraRelative(point, camera.positionWorld(),
                                                           settings.meters_to_render_units);
                    vertices.push_back({{relative.x, relative.y, relative.z}, color});
                }
                uploadAndDrawLineVertices(vertices, GL_LINE_STRIP);
            }
        }
        glBindVertexArray(0);
    }

    void shutdown() noexcept {
        if (window_ != nullptr) {
            glfwMakeContextCurrent(window_);
            glDeleteBuffers(1, &grid_vbo_);
            glDeleteVertexArrays(1, &grid_vao_);
            glDeleteBuffers(1, &trail_vbo_);
            glDeleteVertexArrays(1, &trail_vao_);
            glDeleteBuffers(1, &instance_vbo_);
            glDeleteBuffers(1, &sphere_ebo_);
            glDeleteBuffers(1, &sphere_vbo_);
            glDeleteVertexArrays(1, &sphere_vao_);
            glDeleteProgram(grid_program_);
            glDeleteProgram(sphere_program_);
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        if (glfw_initialized_) {
            glfwTerminate();
            glfw_initialized_ = false;
        }
    }

    GLFWwindow* window_{};
    Camera* active_camera_{};
    bool glfw_initialized_{};
    bool orbiting_{};
    bool panning_{};
    bool left_pressed_in_scene_{};
    bool has_cursor_{};
    double left_press_x_{};
    double left_press_y_{};
    double left_drag_distance_squared_{};
    double last_cursor_x_{};
    double last_cursor_y_{};
    InputCapture input_capture_;
    std::optional<ViewportClick> pending_viewport_click_;
    GLuint sphere_program_{};
    GLuint grid_program_{};
    GLuint sphere_vao_{};
    GLuint sphere_vbo_{};
    GLuint sphere_ebo_{};
    GLuint instance_vbo_{};
    GLuint grid_vao_{};
    GLuint grid_vbo_{};
    GLuint trail_vao_{};
    GLuint trail_vbo_{};
    GLsizei sphere_index_count_{};
    GLsizei grid_vertex_count_{};
    Vec3f grid_origin_{};
    TrailHistory trail_history_;
    std::vector<FieldVectorGlyph> field_glyphs_;
    std::vector<TracedFieldLine> field_lines_;
    std::uint64_t field_cache_revision_{};
    bool field_cache_valid_{};
};

OpenGlRenderer::OpenGlRenderer() : impl_(std::make_unique<Impl>()) {}
OpenGlRenderer::~OpenGlRenderer() = default;
OpenGlRenderer::OpenGlRenderer(OpenGlRenderer&&) noexcept = default;
OpenGlRenderer& OpenGlRenderer::operator=(OpenGlRenderer&&) noexcept = default;

core::Status OpenGlRenderer::initialize(int width, int height, std::string_view title,
                                        bool visible) {
    return impl_->initialize(width, height, title, visible);
}

core::Status OpenGlRenderer::render(const core::Scene& scene, Camera& camera,
                                    const RenderSettings& settings,
                                    const physics::fields::IFieldProvider* fields) {
    return impl_->render(scene, camera, settings, fields);
}

void OpenGlRenderer::present() { impl_->present(); }
void OpenGlRenderer::pollEvents() { impl_->pollEvents(); }
void OpenGlRenderer::setInputCapture(const InputCapture& capture) noexcept {
    impl_->setInputCapture(capture);
}
std::optional<ViewportClick> OpenGlRenderer::takeViewportClick() noexcept {
    return impl_->takeViewportClick();
}
bool OpenGlRenderer::shouldClose() const noexcept { return impl_->shouldClose(); }
void OpenGlRenderer::requestClose() noexcept { impl_->requestClose(); }
void* OpenGlRenderer::nativeWindowHandle() noexcept { return impl_->nativeWindowHandle(); }

} // namespace aetherion::renderer
