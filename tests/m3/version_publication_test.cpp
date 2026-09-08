#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validator.hpp"
#include "automap/version/map_version.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) std::cerr << "失败：" << message << '\n';
    return condition;
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::string read_manifest_hash(const std::filesystem::path& manifest_path) {
    const std::string manifest = read_text(manifest_path);
    const std::string prefix = "\"contentHash\": \"";
    const std::size_t start = manifest.find(prefix);
    if (start == std::string::npos) return {};
    const std::size_t value_start = start + prefix.size();
    const std::size_t end = manifest.find('"', value_start);
    return end == std::string::npos
        ? std::string{}
        : manifest.substr(value_start, end - value_start);
}

}  // namespace

int main() {
    using namespace automap;

    bool passed = true;
    passed &= check(
        version::sha256_hex("abc") ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "SHA-256 实现应通过标准 abc 测试向量。");

    const std::filesystem::path source_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    core::MapData source_map = io::read_canonical_json_file(source_path);
    const core::MapData original_map = source_map;
    const auto report = validation::validate_map(source_map);
    passed &= check(report.can_publish(), "物流园 V0 应通过完整发布质检。");

    const auto unique_suffix = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const std::filesystem::path test_root =
        std::filesystem::temp_directory_path() /
        ("automap_m3_publication_" + std::to_string(unique_suffix));
    const std::filesystem::path version_directory = test_root / "V1";

    const version::VersionPublisher publisher;
    const auto published = publisher.publish(
        source_map, report,
        {
            .output_directory = version_directory,
            .version_id = version::VersionId{"V1"},
            .source_change_set_id = "initial_logistics_park_V1",
            .description = "物流园首个正式地图版本",
            .created_by = "automap_m3_test",
            .published_at_utc = "2026-09-06T00:00:00Z",
        });
    passed &= check(published.published, "无 Fatal/Error 时应成功发布 V1。");
    passed &= check(published.content_hash.starts_with("sha256:") &&
                        published.content_hash.size() == 71U,
                    "发布结果应包含带算法前缀的 SHA-256。");

    for (const std::string_view file : {
             "manifest.json", "map.json", "validation-report.json",
             "initial-changeset.json"}) {
        passed &= check(std::filesystem::is_regular_file(version_directory / file),
                        std::string{file} + " 应写入 V1 目录。");
    }

    const core::MapData reloaded = io::read_canonical_json_file(
        version_directory / "map.json");
    passed &= check(reloaded == original_map, "V1 map.json 应完整保留发布时地图内容。");
    const auto verification = version::verify_map_snapshot(
        version_directory / "map.json", original_map.header.map_id,
        published.content_hash);
    passed &= check(verification.valid, "V1 应可重新读取且内容哈希稳定。");

    const std::string frozen_map_json = read_text(version_directory / "map.json");
    source_map.header.name = "发布后的草稿修改";
    source_map.roads.clear();
    passed &= check(
        version::canonical_content_hash(source_map) != published.content_hash,
        "修改源草稿后其内容哈希应改变。");
    passed &= check(
        read_text(version_directory / "map.json") == frozen_map_json &&
            io::read_canonical_json_file(version_directory / "map.json") == original_map,
        "修改源草稿不得影响已经冻结的 V1。");

    const auto overwrite = publisher.publish(original_map, report, {
        .output_directory = version_directory,
        .version_id = version::VersionId{"V1"},
    });
    passed &= check(!overwrite.published,
                    "已存在的 MapVersion 目录不得被覆盖。");

    core::MapData invalid_map = original_map;
    invalid_map.roads.push_back(invalid_map.roads.front());
    const auto invalid_report = validation::validate_map(invalid_map);
    const std::filesystem::path blocked_directory = test_root / "blocked";
    const auto blocked = publisher.publish(invalid_map, invalid_report, {
        .output_directory = blocked_directory,
        .version_id = version::VersionId{"V1"},
    });
    passed &= check(!blocked.published && !std::filesystem::exists(blocked_directory),
                    "存在 Fatal/Error 时不得创建版本目录。");

    const std::string manifest = read_text(version_directory / "manifest.json");
    const std::string saved_report =
        read_text(version_directory / "validation-report.json");
    passed &= check(manifest.find(published.content_hash) != std::string::npos &&
                        manifest.find("\"status\": \"published\"") != std::string::npos,
                    "manifest 应记录内容哈希和 published 状态。");
    passed &= check(saved_report.find("\"canPublish\": true") != std::string::npos,
                    "发布目录中的质检报告应明确允许发布。");

    const std::filesystem::path repository_version =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "versions" /
        "logistics_park" / "V1";
    const std::string repository_hash =
        read_manifest_hash(repository_version / "manifest.json");
    const auto repository_verification = version::verify_map_snapshot(
        repository_version / "map.json", core::MapId{"logistics_park"},
        repository_hash);
    passed &= check(!repository_hash.empty() && repository_verification.valid,
                    "仓库内 MapVersion V1 应与 manifest 内容哈希一致。");

    std::error_code ignored;
    std::filesystem::remove_all(test_root, ignored);
    if (!passed) return 1;
    std::cout << "AutoMapOps M3 MapVersion V1 发布与不可变性测试通过。\n";
    return 0;
}
