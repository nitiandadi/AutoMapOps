#include "automap/version/version_publisher.hpp"

#include "automap/io/canonical_json.hpp"
#include "automap/validation/validation_report_json.hpp"
#include "automap/version/content_hash.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace automap::version {
namespace {

[[nodiscard]] std::string quote_json(std::string_view value) {
    std::ostringstream output;
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20U) {
                    output << "\\u" << std::hex << std::setw(4)
                           << std::setfill('0') << static_cast<unsigned>(character)
                           << std::dec << std::setfill(' ');
                } else {
                    output << static_cast<char>(character);
                }
        }
    }
    output << '"';
    return output.str();
}

[[nodiscard]] std::string current_utc_timestamp() {
    const std::time_t now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

void write_text_file(const std::filesystem::path& path, std::string_view content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("无法创建文件：" + path.string());
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output) throw std::runtime_error("写入文件失败：" + path.string());
}

[[nodiscard]] std::string manifest_json(
    const core::MapData& map,
    const validation::ValidationReport& report,
    const PublishOptions& options,
    std::string_view source_change_set_id,
    std::string_view timestamp,
    std::string_view content_hash) {
    const auto& coordinate = map.header.coordinate_reference;
    std::ostringstream output;
    output << std::setprecision(17)
           << "{\n"
           << "  \"schemaVersion\": \"1.0\",\n"
           << "  \"mapId\": " << quote_json(map.header.map_id.value()) << ",\n"
           << "  \"version\": " << quote_json(options.version_id.value()) << ",\n"
           << "  \"parentVersion\": ";
    if (options.parent_version) {
        output << quote_json(options.parent_version->value());
    } else {
        output << "null";
    }
    output << ",\n"
           << "  \"sourceChangeSetId\": " << quote_json(source_change_set_id) << ",\n"
           << "  \"status\": \"published\",\n"
           << "  \"description\": " << quote_json(options.description) << ",\n"
           << "  \"mapSchemaVersion\": " << quote_json(map.header.schema_version) << ",\n"
           << "  \"coordinateReference\": {\n"
           << "    \"geodeticDatum\": " << quote_json(coordinate.geodetic_datum) << ",\n"
           << "    \"origin\": {\n"
           << "      \"longitudeDeg\": " << coordinate.origin.longitude_deg << ",\n"
           << "      \"latitudeDeg\": " << coordinate.origin.latitude_deg << ",\n"
           << "      \"altitudeM\": " << coordinate.origin.altitude_m << "\n"
           << "    },\n"
           << "    \"localFrame\": "
           << quote_json(core::local_coordinate_frame_name(coordinate.local_frame)) << ",\n"
           << "    \"linearUnit\": " << quote_json(coordinate.linear_unit) << ",\n"
           << "    \"angleUnit\": " << quote_json(coordinate.angle_unit) << "\n"
           << "  },\n"
           << "  \"objectCounts\": {\n"
           << "    \"roads\": " << map.roads.size() << ",\n"
           << "    \"lanes\": " << map.lanes.size() << ",\n"
           << "    \"laneBoundaries\": " << map.lane_boundaries.size() << ",\n"
           << "    \"junctions\": " << map.junctions.size() << ",\n"
           << "    \"laneConnections\": " << map.lane_connections.size() << ",\n"
           << "    \"operationalAreas\": " << map.operational_areas.size() << ",\n"
           << "    \"stations\": " << map.stations.size() << ",\n"
           << "    \"restrictedAreas\": " << map.restricted_areas.size() << ",\n"
           << "    \"vehicleProfiles\": " << map.vehicle_profiles.size() << "\n"
           << "  },\n"
           << "  \"validationSummary\": {\n"
           << "    \"reportFile\": \"validation-report.json\",\n"
           << "    \"warning\": " << report.count(validation::Severity::warning) << ",\n"
           << "    \"error\": " << report.count(validation::Severity::error) << ",\n"
           << "    \"fatal\": " << report.count(validation::Severity::fatal) << "\n"
           << "  },\n"
           << "  \"contentHash\": " << quote_json(content_hash) << ",\n"
           << "  \"createdAt\": " << quote_json(timestamp) << ",\n"
           << "  \"createdBy\": " << quote_json(options.created_by) << ",\n"
           << "  \"publishedAt\": " << quote_json(timestamp) << ",\n"
           << "  \"publishedBy\": " << quote_json(options.created_by) << "\n"
           << "}\n";
    return output.str();
}

[[nodiscard]] std::string initial_change_set_json(
    const core::MapData& map,
    const PublishOptions& options,
    std::string_view change_set_id,
    std::string_view timestamp,
    std::string_view content_hash) {
    std::ostringstream output;
    output << "{\n"
           << "  \"schemaVersion\": \"1.0\",\n"
           << "  \"changeSetId\": " << quote_json(change_set_id) << ",\n"
           << "  \"mapId\": " << quote_json(map.header.map_id.value()) << ",\n"
           << "  \"baseMapVersion\": null,\n"
           << "  \"type\": \"initial\",\n"
           << "  \"title\": \"创建首个正式地图版本\",\n"
           << "  \"reason\": \"由通过全部发布规则的 Canonical 草稿冻结生成\",\n"
           << "  \"operations\": [\n"
           << "    {\n"
           << "      \"operationId\": \"initial_snapshot\",\n"
           << "      \"action\": \"create\",\n"
           << "      \"target\": {\"objectType\": \"map_snapshot\", \"objectId\": "
           << quote_json(map.header.map_id.value()) << "},\n"
           << "      \"before\": null,\n"
           << "      \"after\": {\"mapFile\": \"map.json\", \"contentHash\": "
           << quote_json(content_hash) << "}\n"
           << "    }\n"
           << "  ],\n"
           << "  \"status\": \"committed\",\n"
           << "  \"validationReportId\": \"validation-report.json\",\n"
           << "  \"createdAt\": " << quote_json(timestamp) << ",\n"
           << "  \"createdBy\": " << quote_json(options.created_by) << ",\n"
           << "  \"approvedAt\": " << quote_json(timestamp) << ",\n"
           << "  \"approvedBy\": " << quote_json(options.created_by) << ",\n"
           << "  \"resultingMapVersion\": " << quote_json(options.version_id.value()) << "\n"
           << "}\n";
    return output.str();
}

}  // namespace

PublishResult VersionPublisher::evaluate(
    const core::MapId& map_id,
    const VersionId& version_id,
    const validation::ValidationReport& report) const {
    PublishResult result{.map_id = map_id, .version_id = version_id};
    if (map_id.empty() || version_id.empty()) {
        result.message = "Map ID 和版本 ID 不能为空。";
        return result;
    }
    if (report.map_id() != map_id) {
        result.message = "质检报告不属于当前地图。";
        return result;
    }
    if (!report.can_publish()) {
        result.message = "存在 Fatal 或 Error，禁止发布地图版本。";
        return result;
    }
    result.published = true;
    result.message = "地图版本具备发布资格。";
    return result;
}

PublishResult VersionPublisher::publish(
    const core::MapData& source_map,
    const validation::ValidationReport& report,
    const PublishOptions& options) const {
    PublishResult result = evaluate(
        source_map.header.map_id, options.version_id, report);
    if (!result.published) return result;
    result.published = false;
    if (options.output_directory.empty()) {
        result.message = "版本输出目录不能为空。";
        return result;
    }

    const std::filesystem::path target =
        std::filesystem::absolute(options.output_directory).lexically_normal();
    result.output_directory = target;
    if (std::filesystem::exists(target)) {
        result.message = "目标版本目录已经存在，MapVersion 不允许覆盖。";
        return result;
    }
    std::filesystem::path staging = target;
    staging += ".publishing";
    if (std::filesystem::exists(staging)) {
        result.message = "发现未完成的发布临时目录，请先人工检查。";
        return result;
    }

    const std::string timestamp = options.published_at_utc.empty()
        ? current_utc_timestamp()
        : options.published_at_utc;
    const std::string change_set_id = options.source_change_set_id.empty()
        ? "initial_" + source_map.header.map_id.value() + "_" +
              options.version_id.value()
        : options.source_change_set_id;

    bool staging_created = false;
    try {
        result.content_hash = canonical_content_hash(source_map);
        std::filesystem::create_directories(staging);
        staging_created = true;
        io::write_canonical_json_file(staging / "map.json", source_map);
        validation::write_validation_report_json_file(
            staging / "validation-report.json", report);
        write_text_file(
            staging / "manifest.json",
            manifest_json(
                source_map, report, options, change_set_id, timestamp,
                result.content_hash));
        write_text_file(
            staging / "initial-changeset.json",
            initial_change_set_json(
                source_map, options, change_set_id, timestamp,
                result.content_hash));
        std::filesystem::rename(staging, target);
    } catch (const std::exception& error) {
        if (staging_created) {
            std::error_code ignored;
            std::filesystem::remove_all(staging, ignored);
        }
        result.message = std::string{"发布失败："} + error.what();
        return result;
    }

    result.published = true;
    result.message = "地图版本发布成功。";
    return result;
}

SnapshotVerificationResult verify_map_snapshot(
    const std::filesystem::path& map_file,
    const core::MapId& expected_map_id,
    std::string_view expected_content_hash) {
    SnapshotVerificationResult result;
    try {
        const core::MapData map = io::read_canonical_json_file(
            map_file, {.expected_map_id = expected_map_id});
        result.content_hash = canonical_content_hash(map);
        if (result.content_hash != expected_content_hash) {
            result.message = "重新读取后的地图内容哈希与发布记录不一致。";
            return result;
        }
        result.valid = true;
        result.message = "地图快照可重新读取且内容哈希稳定。";
        return result;
    } catch (const std::exception& error) {
        result.message = std::string{"验证地图快照失败："} + error.what();
        return result;
    }
}

}  // namespace automap::version
