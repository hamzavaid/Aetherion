#pragma once

#include <string>
#include <utility>
#include <variant>

namespace aetherion::core {

enum class ErrorCode { invalid_argument, numerical_failure, not_found, platform_failure };

struct Error {
    ErrorCode code{};
    std::string message;
};

/// Value-or-error return type used at validation and platform boundaries.
template <typename T> class Result {
  public:
    Result(T value) : storage_(std::move(value)) {}
    Result(Error error) : storage_(std::move(error)) {}

    [[nodiscard]] bool hasValue() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] explicit operator bool() const noexcept { return hasValue(); }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] T& value() { return std::get<T>(storage_); }
    [[nodiscard]] const Error& error() const { return std::get<Error>(storage_); }

  private:
    std::variant<T, Error> storage_;
};

} // namespace aetherion::core
