#include "automap/validation/map_validator.hpp"

#include "automap/validation/basic_geometry_rule.hpp"
#include "automap/validation/connection_geometry_rule.hpp"
#include "automap/validation/network_reachability_rule.hpp"
#include "automap/validation/reference_integrity_rule.hpp"
#include "automap/validation/topology_reciprocity_rule.hpp"
#include "automap/validation/unique_id_rule.hpp"

namespace automap::validation {

ValidationReport validate_map(const core::MapData& map) {
    ValidationReport report{map.header.map_id};
    const ValidationContext context{.map = map};

    const UniqueIdRule unique_id_rule;
    const ReferenceIntegrityRule reference_integrity_rule;
    const BasicGeometryRule basic_geometry_rule;
    const TopologyReciprocityRule topology_reciprocity_rule;
    const ConnectionGeometryRule connection_geometry_rule;
    const NetworkReachabilityRule network_reachability_rule;

    unique_id_rule.validate(context, report);
    reference_integrity_rule.validate(context, report);
    basic_geometry_rule.validate(context, report);
    topology_reciprocity_rule.validate(context, report);
    connection_geometry_rule.validate(context, report);
    network_reachability_rule.validate(context, report);
    return report;
}

}  // namespace automap::validation
