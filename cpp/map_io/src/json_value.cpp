#include "json_value.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <limits>
#include <system_error>

namespace automap::io::detail {
namespace {

class Parser final {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    JsonValue parse() {
        skip_whitespace();
        JsonValue result = parse_value();
        skip_whitespace();
        if (!at_end()) {
            fail("根值结束后存在多余内容");
        }
        return result;
    }

private:
    [[nodiscard]] bool at_end() const noexcept { return position_ >= text_.size(); }
    [[nodiscard]] char peek() const noexcept { return at_end() ? '\0' : text_[position_]; }

    char take() {
        if (at_end()) {
            fail("JSON 意外结束");
        }
        return text_[position_++];
    }

    void skip_whitespace() noexcept {
        while (!at_end()) {
            const char value = peek();
            if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
                break;
            }
            ++position_;
        }
    }

    [[noreturn]] void fail(std::string_view message) const {
        std::size_t line = 1;
        std::size_t column = 1;
        for (std::size_t index = 0; index < position_ && index < text_.size(); ++index) {
            if (text_[index] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        throw JsonSyntaxError(
            "第 " + std::to_string(line) + " 行，第 " + std::to_string(column) +
            " 列：" + std::string(message));
    }

    void expect(char expected) {
        if (take() != expected) {
            fail(std::string("应为 '") + expected + "'");
        }
    }

    void expect_literal(std::string_view literal) {
        for (const char expected : literal) {
            if (take() != expected) {
                fail("无效的 JSON 字面量");
            }
        }
    }

    JsonValue parse_value() {
        switch (peek()) {
        case 'n':
            expect_literal("null");
            return JsonValue{nullptr};
        case 't':
            expect_literal("true");
            return JsonValue{true};
        case 'f':
            expect_literal("false");
            return JsonValue{false};
        case '"':
            return JsonValue{parse_string()};
        case '[':
            return parse_array();
        case '{':
            return parse_object();
        default:
            if (peek() == '-' || (peek() >= '0' && peek() <= '9')) {
                return JsonValue{parse_number()};
            }
            fail("缺少有效的 JSON 值");
        }
    }

    static void append_utf8(std::string& output, std::uint32_t code_point) {
        if (code_point <= 0x7FU) {
            output.push_back(static_cast<char>(code_point));
        } else if (code_point <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        } else if (code_point <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        } else {
            output.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        }
    }

    [[nodiscard]] std::uint32_t parse_hex_quad() {
        std::uint32_t result = 0;
        for (int count = 0; count < 4; ++count) {
            const char digit = take();
            result <<= 4U;
            if (digit >= '0' && digit <= '9') {
                result |= static_cast<std::uint32_t>(digit - '0');
            } else if (digit >= 'a' && digit <= 'f') {
                result |= static_cast<std::uint32_t>(digit - 'a' + 10);
            } else if (digit >= 'A' && digit <= 'F') {
                result |= static_cast<std::uint32_t>(digit - 'A' + 10);
            } else {
                fail("Unicode 转义必须包含四个十六进制数字");
            }
        }
        return result;
    }

    std::string parse_string() {
        expect('"');
        std::string result;
        while (!at_end()) {
            const unsigned char value = static_cast<unsigned char>(take());
            if (value == '"') {
                return result;
            }
            if (value < 0x20U) {
                fail("字符串中不能包含未转义的控制字符");
            }
            if (value != '\\') {
                result.push_back(static_cast<char>(value));
                continue;
            }

            const char escaped = take();
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': {
                std::uint32_t code_point = parse_hex_quad();
                if (code_point >= 0xD800U && code_point <= 0xDBFFU) {
                    if (take() != '\\' || take() != 'u') {
                        fail("高位 Unicode 代理项后缺少低位代理项");
                    }
                    const std::uint32_t low = parse_hex_quad();
                    if (low < 0xDC00U || low > 0xDFFFU) {
                        fail("Unicode 低位代理项无效");
                    }
                    code_point = 0x10000U + ((code_point - 0xD800U) << 10U) +
                                 (low - 0xDC00U);
                } else if (code_point >= 0xDC00U && code_point <= 0xDFFFU) {
                    fail("Unicode 低位代理项前缺少高位代理项");
                }
                append_utf8(result, code_point);
                break;
            }
            default:
                fail("字符串转义无效");
            }
        }
        fail("字符串缺少结束引号");
    }

    double parse_number() {
        const std::size_t begin = position_;
        if (peek() == '-') {
            ++position_;
        }
        if (peek() == '0') {
            ++position_;
            if (peek() >= '0' && peek() <= '9') {
                fail("数字不能包含前导零");
            }
        } else if (peek() >= '1' && peek() <= '9') {
            while (peek() >= '0' && peek() <= '9') {
                ++position_;
            }
        } else {
            fail("数字的整数部分无效");
        }
        if (peek() == '.') {
            ++position_;
            if (peek() < '0' || peek() > '9') {
                fail("小数点后必须包含数字");
            }
            while (peek() >= '0' && peek() <= '9') {
                ++position_;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++position_;
            if (peek() == '+' || peek() == '-') {
                ++position_;
            }
            if (peek() < '0' || peek() > '9') {
                fail("指数部分必须包含数字");
            }
            while (peek() >= '0' && peek() <= '9') {
                ++position_;
            }
        }

        double result = 0.0;
        const char* first = text_.data() + begin;
        const char* last = text_.data() + position_;
        const auto conversion = std::from_chars(first, last, result, std::chars_format::general);
        if (conversion.ec != std::errc{} || conversion.ptr != last || !std::isfinite(result)) {
            fail("数字超出可表示范围");
        }
        return result;
    }

    JsonValue parse_array() {
        expect('[');
        skip_whitespace();
        JsonValue::Array values;
        if (peek() == ']') {
            take();
            return JsonValue{std::move(values)};
        }
        while (true) {
            skip_whitespace();
            values.push_back(parse_value());
            skip_whitespace();
            if (peek() == ']') {
                take();
                return JsonValue{std::move(values)};
            }
            expect(',');
            skip_whitespace();
        }
    }

    JsonValue parse_object() {
        expect('{');
        skip_whitespace();
        JsonValue::Object members;
        if (peek() == '}') {
            take();
            return JsonValue{std::move(members)};
        }
        while (true) {
            skip_whitespace();
            if (peek() != '"') {
                fail("对象成员名必须是字符串");
            }
            std::string name = parse_string();
            for (const auto& [existing_name, unused] : members) {
                static_cast<void>(unused);
                if (existing_name == name) {
                    fail("对象中存在重复成员：" + name);
                }
            }
            skip_whitespace();
            expect(':');
            skip_whitespace();
            members.emplace_back(std::move(name), parse_value());
            skip_whitespace();
            if (peek() == '}') {
                take();
                return JsonValue{std::move(members)};
            }
            expect(',');
            skip_whitespace();
        }
    }

    std::string_view text_;
    std::size_t position_{0};
};

void append_indent(std::string& output, unsigned int depth, unsigned int width) {
    output.append(static_cast<std::size_t>(depth) * width, ' ');
}

void append_escaped_string(std::string& output, std::string_view value) {
    constexpr char hex_digits[] = "0123456789abcdef";
    output.push_back('"');
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (byte < 0x20U) {
                output += "\\u00";
                output.push_back(hex_digits[byte >> 4U]);
                output.push_back(hex_digits[byte & 0x0FU]);
            } else {
                output.push_back(static_cast<char>(byte));
            }
        }
    }
    output.push_back('"');
}

void append_serialized(
    std::string& output,
    const JsonValue& value,
    bool pretty,
    unsigned int width,
    unsigned int depth) {
    if (std::holds_alternative<std::nullptr_t>(value.storage)) {
        output += "null";
    } else if (const auto* boolean = std::get_if<bool>(&value.storage)) {
        output += *boolean ? "true" : "false";
    } else if (const auto* number = std::get_if<double>(&value.storage)) {
        if (!std::isfinite(*number)) {
            throw JsonSyntaxError("不能将非有限浮点数写入 JSON");
        }
        char buffer[64]{};
        const auto result = std::to_chars(
            buffer, buffer + sizeof(buffer), *number,
            std::chars_format::general, std::numeric_limits<double>::max_digits10);
        if (result.ec != std::errc{}) {
            throw JsonSyntaxError("浮点数序列化失败");
        }
        output.append(buffer, result.ptr);
    } else if (const auto* string = std::get_if<std::string>(&value.storage)) {
        append_escaped_string(output, *string);
    } else if (const auto* array = std::get_if<JsonValue::Array>(&value.storage)) {
        output.push_back('[');
        for (std::size_t index = 0; index < array->size(); ++index) {
            if (index != 0) {
                output.push_back(',');
            }
            if (pretty) {
                output.push_back('\n');
                append_indent(output, depth + 1U, width);
            }
            append_serialized(output, (*array)[index], pretty, width, depth + 1U);
        }
        if (pretty && !array->empty()) {
            output.push_back('\n');
            append_indent(output, depth, width);
        }
        output.push_back(']');
    } else {
        const auto& object = std::get<JsonValue::Object>(value.storage);
        output.push_back('{');
        for (std::size_t index = 0; index < object.size(); ++index) {
            if (index != 0) {
                output.push_back(',');
            }
            if (pretty) {
                output.push_back('\n');
                append_indent(output, depth + 1U, width);
            }
            append_escaped_string(output, object[index].first);
            output += pretty ? ": " : ":";
            append_serialized(output, object[index].second, pretty, width, depth + 1U);
        }
        if (pretty && !object.empty()) {
            output.push_back('\n');
            append_indent(output, depth, width);
        }
        output.push_back('}');
    }
}

}  // namespace

JsonValue parse_json(std::string_view text) {
    return Parser{text}.parse();
}

std::string serialize_json(
    const JsonValue& value,
    bool pretty_print,
    unsigned int indent_spaces) {
    std::string result;
    append_serialized(result, value, pretty_print, indent_spaces, 0U);
    if (pretty_print) {
        result.push_back('\n');
    }
    return result;
}

}  // namespace automap::io::detail
