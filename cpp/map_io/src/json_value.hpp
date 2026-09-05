#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace automap::io::detail {

struct JsonValue final {
    using Array = std::vector<JsonValue>;
    using Object = std::vector<std::pair<std::string, JsonValue>>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Storage storage{nullptr};

    JsonValue() = default;
    explicit JsonValue(std::nullptr_t) : storage(nullptr) {}
    explicit JsonValue(bool value) : storage(value) {}
    explicit JsonValue(double value) : storage(value) {}
    explicit JsonValue(std::string value) : storage(std::move(value)) {}
    explicit JsonValue(Array value) : storage(std::move(value)) {}
    explicit JsonValue(Object value) : storage(std::move(value)) {}
};

class JsonSyntaxError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] JsonValue parse_json(std::string_view text);
[[nodiscard]] std::string serialize_json(
    const JsonValue& value,
    bool pretty_print,
    unsigned int indent_spaces);

}  // namespace automap::io::detail
